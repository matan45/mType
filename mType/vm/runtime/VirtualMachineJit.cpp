#include "VirtualMachine.hpp"
#include <cassert>
#include "../../errors/RuntimeException.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../value/SmallArgsBuffer.hpp"
#include "../../value/ValueObject.hpp"
#include "../../value/ValueShim.hpp"
#include "../../value/ObjectInstancePool.hpp"
#include "../jit/JitProfiler.hpp"
#include "../jit/JitCodeCache.hpp"
#include "../jit/JitCompiler.hpp"
#include "../jit/JitContext.hpp"
#include "../jit/OSRManager.hpp"
#include "../jit/LoopProfiler.hpp"
#include "../jit/ic/InlineCacheTable.hpp"
#include "../jit/ic/TypeFeedbackCollector.hpp"
#include "executors/FunctionExecutor.hpp"
#include "executors/ArrayExecutor.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../bytecode/OpCode.hpp"
#include "../../environment/Environment.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
#include <algorithm>
#include <exception>
#include <iostream>

namespace vm::runtime
{
    value::Value VirtualMachine::callFunctionFromJit(const std::string& funcName,
                                                      const std::vector<value::Value>& args)
    {
        auto funcMeta = program->getFunction(funcName);
        if (!funcMeta)
        {
            throw std::runtime_error("JIT interpreter fallback: function not found: " + funcName);
        }

        // Save current interpreter state
        size_t savedIP = instructionPointer;
        size_t savedCallStackDepth = callStack.size();
        size_t savedStackSize = stackManager->size();

        // Push arguments as locals
        for (const auto& arg : args)
        {
            stackManager->push(arg);
        }

        // Initialize remaining locals to null
        for (size_t i = args.size(); i < funcMeta->localCount; ++i)
        {
            stackManager->push(std::monostate{});
        }

        // Create call frame
        CallFrame frame;
        frame.returnAddress = savedIP;
        frame.frameBase = savedStackSize;
        frame.localBase = savedStackSize;
        frame.functionName = program->internFrameName(funcName);
        frame.thisInstance = nullptr;
        pushCallFrame(frame);

        // Track native depth — this function runs its own interpreter loop on the
        // native C++ stack, so it contributes to native recursion depth
        ++jitNativeDepth;

        // Jump to function start
        instructionPointer = funcMeta->startOffset;

        // Run interpreter until this call frame is popped
        // Use post-increment to match interpretLoop pattern (executors set IP = target - 1)
        // Use executionCtx->program so cross-library calls fetch from the correct bytecode
        auto& jitCurrentProgram = executionCtx->program;
        while (callStack.size() > savedCallStackDepth)
        {
            if (instructionPointer >= jitCurrentProgram->getInstructionCount())
            {
                break;
            }

            const auto& instr = jitCurrentProgram->getInstruction(instructionPointer);

            try
            {
                executeInstruction(instr);
            }
            catch (...)
            {
                --jitNativeDepth;
                // Restore state on exception
                while (callStack.size() > savedCallStackDepth)
                {
                    callStack.pop_back();
                }
                instructionPointer = savedIP;
                while (stackManager->size() > savedStackSize)
                {
                    stackManager->pop();
                }
                throw;
            }

            instructionPointer++;
        }

        --jitNativeDepth;

        // Get return value (if any)
        value::Value result = std::monostate{};
        if (stackManager->size() > savedStackSize)
        {
            result = stackManager->pop();
        }

        // Clean up stack to original size
        while (stackManager->size() > savedStackSize)
        {
            stackManager->pop();
        }

        // Restore instruction pointer
        instructionPointer = savedIP;

        return result;
    }

    value::Value VirtualMachine::callMethodFromJit(
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
        const std::string& methodName,
        const std::vector<value::Value>& args)
    {
        if (!program || !instance)
        {
            throw errors::RuntimeException("JIT method fallback: invalid state");
        }

        auto classDef = instance->getClassDefinition();
        size_t argCount = args.size();

        // Extract simple method name (strip class prefix and signature if present)
        std::string simpleMethodName = methodName;
        std::string signaturePart;
        size_t colonPos = methodName.find("::");
        if (colonPos != std::string::npos)
        {
            simpleMethodName = methodName.substr(colonPos + 2);
        }
        size_t slashPos = simpleMethodName.find('/');
        if (slashPos != std::string::npos)
        {
            signaturePart = simpleMethodName.substr(slashPos);
            simpleMethodName = simpleMethodName.substr(0, slashPos);
        }

        // Find method in hierarchy
        auto method = classDef->findInstanceMethodInHierarchy(simpleMethodName, argCount);
        if (!method)
        {
            throw errors::RuntimeException("Instance method not found: " + simpleMethodName +
                " with " + std::to_string(argCount) + " arguments in class " + classDef->getName());
        }

        // Find defining class and type signature
        std::string definingClassName = classDef->getName();
        std::string typeSignature;
        auto currentClass = classDef;
        while (currentClass)
        {
            auto localMethod = currentClass->findInstanceMethod(simpleMethodName, argCount);
            if (localMethod)
            {
                definingClassName = currentClass->getName();
                typeSignature = localMethod->getTypeSignature();
                break;
            }
            currentClass = currentClass->getParentClass();
        }

        // Build qualified name
        std::string qualifiedName = typeSignature.empty()
            ? definingClassName + "::" + simpleMethodName
            : definingClassName + "::" + simpleMethodName + "/" + typeSignature;
        auto* funcMetadata = program->getFunction(qualifiedName);
        if (!funcMetadata)
        {
            throw errors::RuntimeException("Method '" + qualifiedName +
                "' has no bytecode.");
        }

        return callMethodFromJitDirect(instance, qualifiedName, funcMetadata, args);
    }

    value::Value VirtualMachine::callMethodFromJit(
        const value::Value& receiver,
        const std::string& methodName,
        const std::vector<value::Value>& args)
    {
        if (value::isObject(receiver))
        {
            return callMethodFromJit(value::asObject(receiver), methodName, args);
        }

        if (!value::isValueObject(receiver))
        {
            throw errors::RuntimeException(
                "JIT method fallback: receiver is neither ObjectInstance nor ValueObject");
        }

        auto valueObj = value::asValueObject(receiver);
        if (!valueObj)
        {
            throw errors::RuntimeException("JIT method fallback: null ValueObject");
        }
        auto classDef = valueObj->getClassDefinition();
        if (!classDef)
        {
            throw errors::RuntimeException(
                "JIT method fallback: ValueObject has no class definition");
        }

        // Value-class fast-dispatch fix: batch-materialise a temp ObjectInstance
        // from the ValueObject's field vector (no per-field setField hashmap
        // thrash), then reuse the regular-class path. In-method mutation
        // semantics stay identical to the pre-fix behaviour — the tempInstance
        // is independent of the caller's ValueObject, so `this.x = …` writes
        // mutate the temp in place and never propagate back.
        auto tempInstance = value::ObjectInstancePool::getInstance().acquire(classDef);
        tempInstance->loadFromValueObject(*valueObj);

        return callMethodFromJit(tempInstance, methodName, args);
    }

    value::Value VirtualMachine::callMethodFromJitDirect(
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
        const std::string& qualifiedName,
        const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata,
        const std::vector<value::Value>& args,
        const bytecode::BytecodeProgram* calleeProgram)
    {
        if (!program || !instance || !funcMetadata)
        {
            throw errors::RuntimeException("JIT method direct: invalid state");
        }

        size_t argCount = args.size();

        // MYT-182: if the callee lives in a different program (library),
        // resolve its programIndex up front so the frame can carry it for
        // return restoration, and so the mini-interpret loop below runs
        // against the correct bytecode.
        const bytecode::BytecodeProgram* savedProgram = executionCtx->program;
        size_t calleeProgramIndex = callStack.empty() ? 0 : callStack.back().programIndex;
        bool switchedProgram = false;
        if (calleeProgram && calleeProgram != savedProgram)
        {
            for (size_t i = 0; i < loadedPrograms.size(); ++i)
            {
                if (loadedPrograms[i] == calleeProgram)
                {
                    calleeProgramIndex = i;
                    break;
                }
            }
            executionCtx->program = calleeProgram;
            switchedProgram = true;
        }

        // Defining class is the prefix of the qualified name (matches interpretation
        // path that walks the class hierarchy to find the introducing class).
        std::string definingClassName = instance->getClassDefinition()->getName();
        size_t colonPos = qualifiedName.find("::");
        if (colonPos != std::string::npos)
        {
            definingClassName = qualifiedName.substr(0, colonPos);
        }

        // Save current interpreter state (lightweight — no full callStack copy)
        size_t savedIP = instructionPointer;
        size_t savedCallStackDepth = callStack.size();
        size_t savedStackSize = stackManager->size();

        // Push 'this' as first local (slot 0)
        stackManager->push(instance);

        // Push method arguments
        for (const auto& arg : args)
        {
            stackManager->push(arg);
        }

        // Initialize remaining locals to monostate
        size_t pushedSlots = 1 + argCount;
        if (funcMetadata->localCount > pushedSlots)
        {
            size_t additionalLocals = funcMetadata->localCount - pushedSlots;
            for (size_t i = 0; i < additionalLocals; ++i)
            {
                stackManager->push(std::monostate{});
            }
        }

        // Create call frame
        CallFrame frame;
        frame.returnAddress = savedIP;
        frame.frameBase = savedStackSize;
        frame.localBase = savedStackSize;
        // MYT-197: intern on the callee's owning program. Prefer calleeProgram
        // (passed by the JIT caller) when it's set; fall back to whichever
        // program `calleeProgramIndex` resolves to. Never index-dereference
        // loadedPrograms blindly — if the lookup above didn't find calleeProgram
        // in the list, calleeProgramIndex could point at the wrong program.
        const bytecode::BytecodeProgram* ownerProgram =
            calleeProgram ? calleeProgram
                          : (calleeProgramIndex < loadedPrograms.size()
                                 ? loadedPrograms[calleeProgramIndex]
                                 : program);
        frame.functionName = ownerProgram->internFrameName(qualifiedName);
        frame.thisInstance = instance;
        frame.definingClassName = definingClassName;
        // MYT-182: carry the callee's programIndex so the normal return
        // path in ControlFlowExecutor restores context.program correctly.
        frame.programIndex = calleeProgramIndex;
        pushCallFrame(frame);

        // Track native depth
        ++jitNativeDepth;

        // Jump to method start
        instructionPointer = funcMetadata->startOffset;

        // Run interpreter until this call frame is popped
        // Use post-increment to match interpretLoop pattern (executors set IP = target - 1)
        // Use executionCtx->program so cross-library calls fetch from the correct bytecode
        auto& jitCurrentProgram2 = executionCtx->program;
        while (callStack.size() > savedCallStackDepth)
        {
            if (instructionPointer >= jitCurrentProgram2->getInstructionCount())
            {
                break;
            }

            const auto& instr = jitCurrentProgram2->getInstruction(instructionPointer);

            try
            {
                executeInstruction(instr);
            }
            catch (...)
            {
                --jitNativeDepth;
                // Restore state on exception
                while (callStack.size() > savedCallStackDepth)
                {
                    callStack.pop_back();
                }
                instructionPointer = savedIP;
                while (stackManager->size() > savedStackSize)
                {
                    stackManager->pop();
                }
                // MYT-182: restore the pre-call program on exception too.
                if (switchedProgram)
                {
                    executionCtx->program = savedProgram;
                }
                throw;
            }

            instructionPointer++;
        }

        --jitNativeDepth;

        // Get return value (if any)
        value::Value result = std::monostate{};
        if (stackManager->size() > savedStackSize)
        {
            result = stackManager->pop();
        }

        // Clean up stack to original size
        while (stackManager->size() > savedStackSize)
        {
            stackManager->pop();
        }

        // Restore instruction pointer
        instructionPointer = savedIP;

        // MYT-182: restore the pre-call program. The mini-interpret loop
        // above exits on frame-depth check rather than running a RETURN
        // handler, so restore explicitly here.
        if (switchedProgram)
        {
            executionCtx->program = savedProgram;
        }

        return result;
    }

    value::Value VirtualMachine::callMethodFromJitDirect(
        const value::Value& receiver,
        const std::string& qualifiedName,
        const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata,
        const std::vector<value::Value>& args,
        const bytecode::BytecodeProgram* calleeProgram)
    {
        if (value::isObject(receiver))
        {
            return callMethodFromJitDirect(value::asObject(receiver), qualifiedName,
                                           funcMetadata, args, calleeProgram);
        }

        if (!value::isValueObject(receiver))
        {
            throw errors::RuntimeException("JIT method direct: receiver kind not supported");
        }

        auto valueObj = value::asValueObject(receiver);
        if (!valueObj || !valueObj->getClassDefinition())
        {
            throw errors::RuntimeException("JIT method direct: null or classless ValueObject");
        }

        // Same batch-materialisation as callMethodFromJit(Value). Keeps the
        // MYT-184 stack-cookie invariant intact — we still land in the
        // shared_ptr<ObjectInstance> mini-interpret loop, not a direct
        // JIT→JIT dispatch.
        auto tempInstance = value::ObjectInstancePool::getInstance().acquire(valueObj->getClassDefinition());
        tempInstance->loadFromValueObject(*valueObj);

        return callMethodFromJitDirect(tempInstance, qualifiedName, funcMetadata, args, calleeProgram);
    }

    void VirtualMachine::trySpecializeArithmetic(
        const bytecode::BytecodeProgram::Instruction& instr,
        bytecode::OpCode intOpcode, bytecode::OpCode floatOpcode)
    {
        if (!icEnabled || !typeFeedbackCollector || stackManager->size() < 2)
            return;

        typeFeedbackCollector->recordBinaryOp(
            instructionPointer, stackManager->peek(1), stackManager->peek(0));

        if (!typeFeedbackCollector->shouldSpecialize(instructionPointer))
            return;

        auto [lt, rt] = typeFeedbackCollector->getDominantTypes(instructionPointer);
        if (lt == jit::ic::ObservedType::INT && rt == jit::ic::ObservedType::INT)
        {
            const_cast<bytecode::BytecodeProgram::Instruction&>(instr).opcode = intOpcode;
            inlineCacheTable->getTypeFeedback(instructionPointer).specialized = true;

            // MYT-198: try to fuse ADD_INT with a preceding PUSH_INT into
            // ADD_INT_CONST. Only ADD_INT is covered by the fused opcode set
            // for MVP (the same pattern would extend to SUB/MUL/DIV trivially).
            if (intOpcode == bytecode::OpCode::ADD_INT)
            {
                tryFuseAddIntConst();
            }
        }
        else if (lt == jit::ic::ObservedType::FLOAT && rt == jit::ic::ObservedType::FLOAT)
        {
            const_cast<bytecode::BytecodeProgram::Instruction&>(instr).opcode = floatOpcode;
            inlineCacheTable->getTypeFeedback(instructionPointer).specialized = true;
        }
    }

    void VirtualMachine::trySpecializeBitwise(
        const bytecode::BytecodeProgram::Instruction& instr,
        bytecode::OpCode intOpcode)
    {
        if (!icEnabled || !typeFeedbackCollector || stackManager->size() < 2)
            return;

        typeFeedbackCollector->recordBinaryOp(
            instructionPointer, stackManager->peek(1), stackManager->peek(0));

        if (!typeFeedbackCollector->shouldSpecialize(instructionPointer))
            return;

        auto [lt, rt] = typeFeedbackCollector->getDominantTypes(instructionPointer);
        if (lt == jit::ic::ObservedType::INT && rt == jit::ic::ObservedType::INT)
        {
            const_cast<bytecode::BytecodeProgram::Instruction&>(instr).opcode = intOpcode;
            inlineCacheTable->getTypeFeedback(instructionPointer).specialized = true;
        }
    }

    void VirtualMachine::trySpecializeBitwiseUnary(
        const bytecode::BytecodeProgram::Instruction& instr,
        bytecode::OpCode intOpcode)
    {
        if (!icEnabled || !typeFeedbackCollector || stackManager->size() < 1)
            return;

        // Reuse recordBinaryOp with the same operand twice — the feedback
        // collector only inspects tag, and duplicating avoids adding a
        // separate unary path for a one-operand promotion.
        const value::Value& tos = stackManager->peek(0);
        typeFeedbackCollector->recordBinaryOp(instructionPointer, tos, tos);

        if (!typeFeedbackCollector->shouldSpecialize(instructionPointer))
            return;

        auto [lt, rt] = typeFeedbackCollector->getDominantTypes(instructionPointer);
        if (lt == jit::ic::ObservedType::INT && rt == jit::ic::ObservedType::INT)
        {
            const_cast<bytecode::BytecodeProgram::Instruction&>(instr).opcode = intOpcode;
            inlineCacheTable->getTypeFeedback(instructionPointer).specialized = true;
        }
    }

    void VirtualMachine::tryFuseAddIntConst()
    {
        const size_t ip = instructionPointer;
        if (ip == 0) return;

        // Prior must be PUSH_INT; its operand[0] is a constant-pool index
        // (see StackOperationsExecutor::handlePushInt). ADD_INT_CONST stores
        // that same index in fusedSlot and reads the literal from the pool
        // at dispatch time — no eager resolution.
        const auto& prev = program->getInstruction(ip - 1);
        if (prev.opcode != bytecode::OpCode::PUSH_INT) return;
        if (prev.operands.empty()) return;

        // Same fusion-safety gates as the LOAD_LOCAL fusions: no control-flow
        // target may land directly on the fused op, and sticky un-fuse blocks
        // re-fusion.
        if (program->isFusionUnsafeTarget(ip)) return;

        // MYT-201: fused state lives in the per-IP side table. Sticky un-fuse
        // gate reads via findCachedState so a never-fused site doesn't
        // allocate an entry just to check the default 0.
        if (auto* existing = program->findCachedState(ip))
        {
            if (existing->fusedDeoptCount >= 1) return;
        }

        uint64_t constIdx = prev.operands[0];
        // fusedSlot is uint32_t — assert before truncation so an oversized
        // constant-pool index surfaces as a clean failure rather than silent
        // data loss. Constant pools are bounded by the compiler, but the
        // cast is attacker-reachable via a malformed .mtc operand.
        assert(constIdx <= UINT32_MAX &&
               "ADD_INT_CONST fusion: PUSH_INT operand exceeds fusedSlot width");
        auto& prevMut = const_cast<bytecode::BytecodeProgram*>(program)
                            ->getMutableInstruction(ip - 1);
        prevMut.opcode = bytecode::OpCode::NOP;
        prevMut.operands.clear();

        auto& state = program->getOrCreateCachedState(ip);
        state.fusedSlot = static_cast<uint32_t>(constIdx);

        auto& mut = const_cast<bytecode::BytecodeProgram*>(program)
                        ->getMutableInstruction(ip);
        mut.opcode = bytecode::OpCode::ADD_INT_CONST;
    }

    void VirtualMachine::executeCallWithJit(
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        using OpCode = bytecode::OpCode;

        if (jitEnabled && jitCodeCache && jitNativeDepth < MAX_JIT_NATIVE_DEPTH)
        {
            std::string funcName = program->getConstantPool().getString(instr.operands[0]);
            auto jitCode = jitCodeCache->lookup(funcName);
            if (jitCode)
            {
                size_t argCount = instr.operands[1];

                // MYT-196: small-buffer-optimized args for JIT entry.
                value::SmallArgsBuffer args(argCount);
                for (size_t i = argCount; i > 0; --i)
                    args[i - 1] = stackManager->pop();

                jit::JitContext jitCtx{};
                jitCtx.args = args.data();
                jitCtx.argCount = args.size();
                jitCtx.hasReturnValue = false;
                jitCtx.program = program;
                jitCtx.stackManager = stackManager.get();
                jitCtx.environment = environment.get();
                jitCtx.vm = this;
                jitCtx.jitCodeCache = jitCodeCache.get();
                jitCtx.icTable = inlineCacheTable.get();

                size_t sepPos = funcName.find("::");
                if (sepPos != std::string::npos)
                    jitCtx.callingClassName = funcName.substr(0, sepPos);

                ++jitNativeDepth;
                jitCode(&jitCtx);
                --jitNativeDepth;

                // Rethrow any exception that was caught inside JIT helpers
                // (exceptions can't unwind through asmjit-generated frames)
                if (jitCtx.pendingException)
                    std::rethrow_exception(jitCtx.pendingException);

                if (jitCtx.hasReturnValue)
                    stackManager->push(jitCtx.returnValue);

                stats.functionCalls++;
                return;
            }
        }
        // Fall back to interpreter (uses managed call stack with overflow protection)
        functionExecutor->handleCall(instr);
    }

    void VirtualMachine::executeCallFastWithJit(
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (jitEnabled && jitCodeCache && jitNativeDepth < MAX_JIT_NATIVE_DEPTH)
        {
            const auto* funcMeta = program->getFunctionByIndex(instr.operands[0]);
            if (funcMeta) {
                auto jitCode = jitCodeCache->lookup(funcMeta->mangledName);
                if (jitCode)
                {
                    size_t argCount = instr.operands[1];

                    // MYT-196: small-buffer-optimized args for JIT entry.
                    value::SmallArgsBuffer args(argCount);
                    for (size_t i = argCount; i > 0; --i)
                        args[i - 1] = stackManager->pop();

                    jit::JitContext jitCtx{};
                    jitCtx.args = args.data();
                    jitCtx.argCount = args.size();
                    jitCtx.hasReturnValue = false;
                    jitCtx.program = program;
                    jitCtx.stackManager = stackManager.get();
                    jitCtx.environment = environment.get();
                    jitCtx.vm = this;
                    jitCtx.jitCodeCache = jitCodeCache.get();
                    jitCtx.icTable = inlineCacheTable.get();

                    size_t sepPos = funcMeta->name.find("::");
                    if (sepPos != std::string::npos)
                        jitCtx.callingClassName = funcMeta->name.substr(0, sepPos);

                    ++jitNativeDepth;
                    jitCode(&jitCtx);
                    --jitNativeDepth;

                    if (jitCtx.pendingException)
                        std::rethrow_exception(jitCtx.pendingException);

                    if (jitCtx.hasReturnValue)
                        stackManager->push(jitCtx.returnValue);

                    stats.functionCalls++;
                    return;
                }
            }
        }
        functionExecutor->handleCallFast(instr);
    }

    void VirtualMachine::printJitStats() const
    {
        std::cout << "\n=== JIT Statistics ===\n";

        if (!jitEnabled)
        {
            std::cout << "  JIT disabled\n";
            std::cout << "======================\n";
            return;
        }

        // Function profiling
        std::cout << "Function Profiling:\n";
        if (jitProfiler)
        {
            std::cout << "  Hot threshold:          " << jitProfiler->getHotThreshold() << " calls\n";
            const auto& hotFuncs = jitProfiler->getHotFunctions();
            std::cout << "  Hot functions:          " << hotFuncs.size() << "\n";
            for (const auto& name : hotFuncs)
            {
                uint32_t calls = jitProfiler->getInvocationCount(name);
                bool compiled = jitCodeCache && jitCodeCache->contains(name);
                std::cout << "    - " << name << " (" << calls << " calls)"
                          << (compiled ? " [compiled]" : " [bailout]") << "\n";
            }
        }

        // Compilation stats
        std::cout << "Compilation:\n";
        if (jitCompiler)
        {
            std::cout << "  Successful compiles:    " << jitCompiler->getCompileCount() << "\n";
            std::cout << "  Bailouts:               " << jitCompiler->getBailoutCount() << "\n";
        }
        if (jitCodeCache)
            std::cout << "  Cached functions:       " << jitCodeCache->size() << "\n";

        // MYT-172 AC #3: inline field-IC stats. Hits = JIT fast-path took
        // the inlined shape-guard + indexed load/store. Misses = shape
        // guard failed and the slow-path helper handled the access.
        // MYT-191: SET counters split out separately — GET misses are
        // dominated by non-primitive tags, SET misses by ValueObject CoW,
        // merging them hides regressions.
        if (jitCompiler)
        {
            std::cout << "Field IC (GET):\n";
            std::cout << "  Inline hits:            "
                      << jitCompiler->getInlineFieldICHits() << "\n";
            std::cout << "  Inline misses:          "
                      << jitCompiler->getInlineFieldICMisses() << "\n";
            std::cout << "Field IC (SET):\n";
            std::cout << "  Inline hits:            "
                      << jitCompiler->getInlineFieldSetICHits() << "\n";
            std::cout << "  Inline misses:          "
                      << jitCompiler->getInlineFieldSetICMisses() << "\n";

            // MYT-210 (fills MYT-179 stub): per-decision inline-eligibility
            // breakdown for both method-call and plain-function inlining.
            // Print only non-zero rows to keep output compact; if every row
            // is zero (no JIT-compiled function reached an inlineable site),
            // skip the section entirely.
            const auto& dec = jitCompiler->getInlineDecisions();
            auto anyNonZero = [](const std::array<uint64_t, jit::InlineDecisionCounters::SIZE>& a) {
                for (auto v : a) if (v) return true;
                return false;
            };
            auto printSection = [](const char* label,
                                    const std::array<uint64_t, jit::InlineDecisionCounters::SIZE>& a) {
                std::cout << label << "\n";
                for (size_t i = 0; i < jit::InlineDecisionCounters::SIZE; ++i)
                {
                    if (!a[i]) continue;
                    const auto reason = static_cast<optimization::InlineDecision>(i);
                    std::cout << "  " << optimization::inlineDecisionName(reason)
                              << ": " << a[i] << "\n";
                }
            };
            if (anyNonZero(dec.perReasonMethod))
                printSection("Inline decisions (method):", dec.perReasonMethod);
            if (anyNonZero(dec.perReasonFunction))
                printSection("Inline decisions (function):", dec.perReasonFunction);
        }

        // Loop OSR stats
        std::cout << "Loop OSR:\n";
        if (osrManager)
        {
            const auto& loopProfiler = osrManager->getLoopProfiler();
            const auto& profiles = loopProfiler.getProfiles();
            size_t compiled = 0, failed = 0;
            for (const auto& [id, profile] : profiles)
            {
                if (profile.osrCompiled) compiled++;
                if (profile.osrFailed) failed++;
            }
            std::cout << "  OSR threshold:          " << loopProfiler.getOsrThreshold() << " iterations\n";
            std::cout << "  Loops profiled:         " << profiles.size() << "\n";
            std::cout << "  OSR compiled:           " << compiled << "\n";
            std::cout << "  OSR failed:             " << failed << "\n";

            // MYT-148: per-loop bailout reason breakdown. Answers "why didn't
            // this loop tier up?" without a debugger. Shows the offending
            // opcode mnemonic for UNSUPPORTED_OPCODE / CODEGEN_FAILURE so the
            // next step (remediation) is obvious.
            if (failed > 0)
            {
                std::cout << "  Failed loops:\n";
                for (const auto& [id, profile] : profiles)
                {
                    if (!profile.osrFailed) continue;
                    std::cout << "    - offset 0x" << std::hex << id.jumpBackOffset
                              << std::dec << ": "
                              << jit::osrBailoutReasonName(profile.bailoutReason);
                    if (profile.bailoutReason == jit::OSRBailoutReason::UNSUPPORTED_OPCODE ||
                        profile.bailoutReason == jit::OSRBailoutReason::CODEGEN_FAILURE)
                    {
                        std::cout << " (0x" << std::hex
                                  << static_cast<unsigned>(profile.offendingOpcode)
                                  << std::dec << " = "
                                  << bytecode::getOpCodeName(
                                         static_cast<bytecode::OpCode>(profile.offendingOpcode))
                                  << ")";
                    }
                    std::cout << "\n";
                }
            }
        }

        std::cout << "======================\n";
    }

    value::Value VirtualMachine::createMultiArrayFromJit(uint32_t typeNameIndex,
                                                          const std::vector<int64_t>& dimensions,
                                                          size_t totalDimensions)
    {
        if (!program)
        {
            throw errors::RuntimeException("JIT multi-array fallback: no program loaded");
        }
        const std::string& elementTypeName =
            program->getConstantPool().getString(typeNameIndex);
        auto classRegistry = environment ? environment->getClassRegistry().get() : nullptr;
        return ArrayExecutor::buildMultiArray(classRegistry, elementTypeName, dimensions, totalDimensions);
    }
}
