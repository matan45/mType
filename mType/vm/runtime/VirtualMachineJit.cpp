#include "VirtualMachine.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
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
        frame.functionName = funcName;
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
        frame.functionName = qualifiedName;
        frame.thisInstance = instance;
        frame.definingClassName = definingClassName;
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
        }
        else if (lt == jit::ic::ObservedType::FLOAT && rt == jit::ic::ObservedType::FLOAT)
        {
            const_cast<bytecode::BytecodeProgram::Instruction&>(instr).opcode = floatOpcode;
            inlineCacheTable->getTypeFeedback(instructionPointer).specialized = true;
        }
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

                std::vector<value::Value> args;
                args.reserve(argCount);
                for (size_t i = 0; i < argCount; ++i)
                    args.push_back(stackManager->pop());
                std::reverse(args.begin(), args.end());

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

                    std::vector<value::Value> args;
                    args.reserve(argCount);
                    for (size_t i = 0; i < argCount; ++i)
                        args.push_back(stackManager->pop());
                    std::reverse(args.begin(), args.end());

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
            // opcode mnemonic for UNSUPPORTED_OPCODE / BAILOUT_OPCODE so the
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
                        profile.bailoutReason == jit::OSRBailoutReason::BAILOUT_OPCODE ||
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
