#include "OSRManager.hpp"
#include "JitHelpers.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "../bytecode/OpCode.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/StringPool.hpp"
#include <new>

namespace vm::jit
{
    OSRManager::OSRManager() {}
    OSRManager::~OSRManager() {}

    bool OSRManager::tryOSR(size_t jumpBackOffset,
                             const bytecode::BytecodeProgram& program,
                             vm::runtime::ExecutionContext& context,
                             vm::runtime::VirtualMachine& vm,
                             JitCompiler& compiler,
                             JitCodeCache& codeCache)
    {
        LoopId loopId{jumpBackOffset};

        // Check if we already have compiled code for this loop
        auto cacheIt = osrCache.find(jumpBackOffset);
        if (cacheIt != osrCache.end())
        {
            // Already compiled — capture fresh state and execute
            OSRState state;
            if (!captureState(state, jumpBackOffset, program, context))
            {
                return false;
            }
            return executeOSRLoop(cacheIt->second, state, program, context, vm, codeCache);
        }

        // Profile the loop — returns true on exact threshold crossing
        if (!loopProfiler.recordIteration(loopId))
        {
            return false;
        }

        // Loop just became hot — try to compile it
        OSRState state;
        if (!captureState(state, jumpBackOffset, program, context))
        {
            loopProfiler.markFailed(loopId);
            return false;
        }

        // Attempt JIT compilation of the loop
        std::string osrKey = "osr@" + std::to_string(jumpBackOffset);
        bool compiled = compiler.compileLoopOSR(
            state.loopStartOffset,
            state.loopEndOffset,
            state.jumpBackOffset,
            state.locals,
            state.localCount,
            program,
            codeCache);

        if (!compiled)
        {
            loopProfiler.markFailed(loopId);
            return false;
        }

        loopProfiler.markCompiled(loopId);

        // Retrieve compiled function
        auto fn = codeCache.lookup(osrKey);
        if (!fn)
        {
            loopProfiler.markFailed(loopId);
            return false;
        }

        // Store in our local cache for fast lookup on subsequent iterations
        osrCache[jumpBackOffset] = fn;

        // Execute the compiled loop
        return executeOSRLoop(fn, state, program, context, vm, codeCache);
    }

    bool OSRManager::captureState(OSRState& state,
                                   size_t jumpBackOffset,
                                   const bytecode::BytecodeProgram& program,
                                   vm::runtime::ExecutionContext& context)
    {
        // Find enclosing LOOP_START by scanning backward
        size_t loopStartOffset = SIZE_MAX;
        for (size_t ip = jumpBackOffset; ip != SIZE_MAX; --ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (instr.opcode == bytecode::OpCode::LOOP_START)
            {
                loopStartOffset = ip;
                break;
            }
            if (ip == 0) break;
        }
        if (loopStartOffset == SIZE_MAX)
        {
            return false;
        }

        // Find enclosing LOOP_END by scanning forward
        size_t loopEndOffset = SIZE_MAX;
        for (size_t ip = jumpBackOffset + 1; ip < program.getInstructionCount(); ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (instr.opcode == bytecode::OpCode::LOOP_END)
            {
                loopEndOffset = ip;
                break;
            }
        }
        if (loopEndOffset == SIZE_MAX)
        {
            return false;
        }

        // Reject if in a function with lambda captures (shared frame)
        if (!context.callStack.empty() && context.callStack.back().sharedFrame)
        {
            return false;
        }

        // Get function metadata for local count
        std::string functionName;
        size_t localCount = 0;
        if (!context.callStack.empty())
        {
            functionName = context.callStack.back().functionName;
            auto funcMeta = program.getFunction(functionName);
            if (funcMeta)
            {
                localCount = funcMeta->localCount;
            }
        }
        else
        {
            // Top-level script: estimate localCount from the stack
            // For top-level, localBase is 0 and we use the frame base
            localCount = 0;
        }

        if (localCount == 0)
        {
            return false;
        }

        // Capture locals
        size_t localBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        state.locals.clear();
        state.locals.reserve(localCount);

        for (size_t i = 0; i < localCount; ++i)
        {
            size_t stackIdx = localBase + i;
            if (stackIdx < context.stackManager->size())
            {
                const value::Value& val = context.stackManager->getStack()[stackIdx];
                state.locals.push_back(LocalSlotInfo{i, inferSlotType(val), val});
            }
            else
            {
                // Uninitialized local
                state.locals.push_back(LocalSlotInfo{i, SlotType::INT, value::Value{int64_t(0)}});
            }
        }

        // Check operand stack above locals (should be empty at JUMP_BACK for V1)
        size_t operandStackBase = localBase + localCount;
        if (context.stackManager->size() > operandStackBase)
        {
            // Non-empty operand stack — reject for V1
            return false;
        }

        state.loopStartOffset = loopStartOffset;
        state.loopEndOffset = loopEndOffset;
        state.jumpBackOffset = jumpBackOffset;
        state.loopConditionOffset = loopStartOffset + 1;
        state.localCount = localCount;
        state.functionName = functionName;
        state.resumeOffset = loopEndOffset + 1;

        return true;
    }

    bool OSRManager::executeOSRLoop(OSRLoopFunction func,
                                     const OSRState& state,
                                     const bytecode::BytecodeProgram& program,
                                     vm::runtime::ExecutionContext& context,
                                     vm::runtime::VirtualMachine& vm,
                                     JitCodeCache& codeCache)
    {
        // Allocate input/output Value arrays
        std::vector<value::Value> inputLocals(state.localCount);
        std::vector<value::Value> outputLocals(state.localCount);

        // Copy captured locals into input array
        for (const auto& slotInfo : state.locals)
        {
            if (slotInfo.slot < state.localCount)
            {
                inputLocals[slotInfo.slot] = slotInfo.value;
            }
        }

        // Initialize output locals to same values (for deopt safety)
        for (size_t i = 0; i < state.localCount; ++i)
        {
            outputLocals[i] = inputLocals[i];
        }

        // Build JitContext
        JitContext jitCtx{};
        jitCtx.args = nullptr;
        jitCtx.argCount = 0;
        jitCtx.hasReturnValue = false;
        jitCtx.program = &program;
        jitCtx.stackManager = context.stackManager.get();
        jitCtx.environment = context.environment.get();
        jitCtx.vm = &vm;
        jitCtx.jitCodeCache = &codeCache;
        jitCtx.icTable = vm.getInlineCacheTable();

        // Extract calling class name for access validation
        {
            size_t sepPos = state.functionName.find("::");
            if (sepPos != std::string::npos)
                jitCtx.callingClassName = state.functionName.substr(0, sepPos);
        }

        jitCtx.osrLocals = inputLocals.data();
        jitCtx.osrLocalCount = state.localCount;
        jitCtx.osrOutputLocals = outputLocals.data();
        jitCtx.osrExitOffset = 0;
        jitCtx.osrExited = false;

        try
        {
            func(&jitCtx);

            if (jitCtx.osrExited)
            {
                lastResult.success = true;
                lastResult.deoptimized = false;
                lastResult.resumeIP = jitCtx.osrExitOffset;
                lastResult.updatedLocals = std::move(outputLocals);
                writeBackState(lastResult, state, context);
                return true;
            }

            // Loop function returned without setting osrExited — shouldn't happen
            // but treat as fallback
            return false;
        }
        catch (const OSRDeoptException& e)
        {
            // Deoptimization — restore state and fall back to interpreter
            guards::DeoptState deoptState;
            deoptState.reason = guards::DeoptReason::EXCEPTION_THROWN;
            deoptState.bytecodeOffset = e.bytecodeOffset;
            deoptState.locals = std::move(outputLocals);

            guards::DeoptimizationHandler::handleDeopt(deoptState, state, context);

            lastResult.success = false;
            lastResult.deoptimized = true;
            lastResult.resumeIP = e.bytecodeOffset;
            return true;
        }
        catch (...)
        {
            // Any other exception — write back whatever state we have
            // and let the interpreter handle it
            lastResult.success = false;
            lastResult.deoptimized = true;
            lastResult.resumeIP = state.jumpBackOffset;

            // Write back original locals (unchanged since we don't know where we failed)
            lastResult.updatedLocals.clear();
            for (const auto& slot : state.locals)
            {
                lastResult.updatedLocals.push_back(slot.value);
            }
            writeBackState(lastResult, state, context);

            // Re-throw to let the VM handle the exception
            throw;
        }
    }

    void OSRManager::writeBackState(const OSRResult& result,
                                     const OSRState& state,
                                     vm::runtime::ExecutionContext& context)
    {
        size_t localBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;

        // Write each updated local back to the interpreter stack
        for (size_t i = 0; i < result.updatedLocals.size() && i < state.localCount; ++i)
        {
            size_t stackIdx = localBase + i;
            if (stackIdx < context.stackManager->size())
            {
                context.stackManager->getStack()[stackIdx] = result.updatedLocals[i];
            }
        }

        // Set instruction pointer: -1 because the interpreter loop increments
        context.instructionPointer = result.resumeIP - 1;
    }

    SlotType OSRManager::inferSlotType(const value::Value& val)
    {
        if (std::holds_alternative<int64_t>(val)) return SlotType::INT;
        if (std::holds_alternative<float>(val)) return SlotType::FLOAT;
        if (std::holds_alternative<bool>(val)) return SlotType::BOOL;
        if (std::holds_alternative<std::string>(val)) return SlotType::STRING;
        if (std::holds_alternative<value::InternedString>(val)) return SlotType::STRING;
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) return SlotType::OBJECT;
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(val)) return SlotType::ARRAY;
        return SlotType::BOXED;
    }
}
