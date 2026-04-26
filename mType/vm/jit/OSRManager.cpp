#include "OSRManager.hpp"
#include "JitHelpers.hpp"
#include "../../value/ValueShim.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "../bytecode/OpCode.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../value/NativeArray.hpp"
#include "../../value/StringPool.hpp"
#include <iostream>      // MYT-XXX DEBUG
#include <cstdlib>       // MYT-XXX DEBUG (getenv)

namespace vm::jit
{
    
    OSRManager::OSRManager() {}
    OSRManager::~OSRManager() {}

    bool OSRManager::compileAndCacheLoop(OSRState& state,
                                          size_t jumpBackOffset,
                                          const bytecode::BytecodeProgram& program,
                                          JitCompiler& compiler,
                                          JitCodeCache& codeCache,
                                          ic::TypeFeedbackCollector* typeFeedback,
                                          OSRBailoutReason& outReason,
                                          uint8_t& outOffendingOpcode)
    {
        std::string osrKey = "osr@" + std::to_string(jumpBackOffset);
        outReason = OSRBailoutReason::NONE;
        outOffendingOpcode = 0;

        bool compiled = compiler.compileLoopOSR(
            state.loopStartOffset,
            state.loopEndOffset,
            state.jumpBackOffset,
            state.locals,
            state.localCount,
            program,
            codeCache,
            typeFeedback,
            &outReason,
            &outOffendingOpcode);

        if (!compiled)
        {
            return false;
        }

        auto fn = codeCache.lookup(osrKey);
        if (!fn)
        {
            // compileLoopOSR claimed success but the cache doesn't have the
            // entry - treat as a generic codegen failure.
            outReason = OSRBailoutReason::CODEGEN_FAILURE;
            return false;
        }

        osrCache[jumpBackOffset] = fn;
        return true;
    }

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
            OSRState state;
            if (captureState(state, jumpBackOffset, program, context) != OSRBailoutReason::NONE)
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

        // Loop just became hot — try to compile it.
        // MYT-148: thread the specific bailout reason out of captureState and
        // through compileAndCacheLoop so --jit-stats can show which gate rejected.
        OSRState state;
        OSRBailoutReason captureReason =
            captureState(state, jumpBackOffset, program, context);
        if (captureReason != OSRBailoutReason::NONE)
        {
            loopProfiler.markFailed(loopId, captureReason);
            return false;
        }

        OSRBailoutReason compileReason = OSRBailoutReason::NONE;
        uint8_t compileOpcode = 0;
        if (!compileAndCacheLoop(state, jumpBackOffset, program, compiler, codeCache,
                                 vm.getTypeFeedbackCollector(),
                                 compileReason, compileOpcode))
        {
            loopProfiler.markFailed(loopId,
                                     compileReason == OSRBailoutReason::NONE
                                       ? OSRBailoutReason::CODEGEN_FAILURE
                                       : compileReason,
                                     compileOpcode);
            return false;
        }

        loopProfiler.markCompiled(loopId);
        return executeOSRLoop(osrCache[jumpBackOffset], state, program, context, vm, codeCache);
    }

    bool OSRManager::findLoopBoundaries(size_t jumpBackOffset,
                                         const bytecode::BytecodeProgram& program,
                                         size_t& loopStartOffset,
                                         size_t& loopEndOffset)
    {
        // Nesting-balanced scan. The back-edge sits inside exactly one loop
        // body; the enclosing LOOP_START/LOOP_END are the first unmatched
        // markers walking outward. A naive "first LOOP_START backward,
        // first LOOP_END forward" picks inner-loop markers for an outer
        // back-edge and clips the compile range (MYT-153 Bug #1).
        loopStartOffset = SIZE_MAX;
        {
            size_t depth = 0;
            for (size_t ip = jumpBackOffset; ; --ip)
            {
                const auto& instr = program.getInstruction(ip);
                if (instr.opcode == bytecode::OpCode::LOOP_END)
                {
                    ++depth;
                }
                else if (instr.opcode == bytecode::OpCode::LOOP_START)
                {
                    if (depth == 0)
                    {
                        loopStartOffset = ip;
                        break;
                    }
                    --depth;
                }
                if (ip == 0) break;
            }
        }
        if (loopStartOffset == SIZE_MAX)
        {
            return false;
        }

        loopEndOffset = SIZE_MAX;
        {
            size_t depth = 1;
            for (size_t ip = jumpBackOffset + 1; ip < program.getInstructionCount(); ++ip)
            {
                const auto& instr = program.getInstruction(ip);
                if (instr.opcode == bytecode::OpCode::LOOP_START)
                {
                    ++depth;
                }
                else if (instr.opcode == bytecode::OpCode::LOOP_END)
                {
                    --depth;
                    if (depth == 0)
                    {
                        loopEndOffset = ip;
                        break;
                    }
                }
            }
        }
        return loopEndOffset != SIZE_MAX;
    }

    void OSRManager::captureLocals(OSRState& state,
                                    size_t localBase,
                                    size_t localCount,
                                    vm::runtime::ExecutionContext& context)
    {
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
    }

    OSRBailoutReason OSRManager::captureState(OSRState& state,
                                                size_t jumpBackOffset,
                                                const bytecode::BytecodeProgram& program,
                                                vm::runtime::ExecutionContext& context)
    {
        size_t loopStartOffset, loopEndOffset;
        if (!findLoopBoundaries(jumpBackOffset, program, loopStartOffset, loopEndOffset))
            return OSRBailoutReason::LOOP_MARKERS_MISSING;

        // Reject if in a function with real lambda captures.
        //
        // We can't just test `sharedFrame != nullptr`: VariableExecutor
        // pre-allocates a SharedStackFrame on every STORE_LOCAL that carries
        // a variable name (which is essentially every local store the
        // compiler emits) as speculative bookkeeping in case a lambda later
        // captures that local. Using `!= nullptr` would bail out on every
        // ordinary function/script and block OSR across the whole benchmark
        // suite.
        //
        // A frame is only *truly* shared when either:
        //   (a) it is itself a lambda invocation frame — locals may resolve
        //       through `sharedFrame->parentFrame` instead of the operand
        //       stack the OSR captured-state assumes; or
        //   (b) an outside owner (captured lambda, VM interop path) holds a
        //       reference to the SharedStackFrame, so mutations can happen
        //       outside native code's view.
        if (!context.callStack.empty())
        {
            const auto& frame = context.callStack.back();
            if (frame.originatingLambda)
                return OSRBailoutReason::SHARED_FRAME_REJECTION;
            if (frame.sharedFrame && frame.sharedFrame.use_count() > 1)
                return OSRBailoutReason::SHARED_FRAME_REJECTION;
        }

        // Get function metadata for local count.
        // MYT-197: resolve the interned handle via the callee's program; OSR
        // only runs on the frame's own program (no cross-program tier-up).
        std::string functionName;
        size_t localCount = 0;
        if (!context.callStack.empty())
        {
            const auto& frame = context.callStack.back();
            functionName = program.getFrameName(frame.functionName);
            auto funcMeta = program.getFunctionMeta(frame.functionName);
            if (funcMeta)
            {
                localCount = funcMeta->localCount;
            }
            else if (functionName == "__script_main__")
            {
                // Top-level script body isn't registered as a function; the
                // compiler publishes the local count out-of-band so OSR can
                // still tier up script-scope loops.
                localCount = program.getTopLevelLocalCount();
            }
        }

        if (localCount == 0)
            return OSRBailoutReason::NO_FUNCTION_FRAME;

        // Capture locals
        size_t localBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        captureLocals(state, localBase, localCount, context);

        // Check operand stack above locals (should be empty at JUMP_BACK for V1)
        size_t operandStackBase = localBase + localCount;
        if (context.stackManager->size() > operandStackBase)
            return OSRBailoutReason::OPERAND_STACK_NOT_EMPTY;

        state.loopStartOffset = loopStartOffset;
        state.loopEndOffset = loopEndOffset;
        state.jumpBackOffset = jumpBackOffset;
        state.loopConditionOffset = loopStartOffset + 1;
        state.localCount = localCount;
        state.functionName = functionName;
        state.resumeOffset = loopEndOffset + 1;

        return OSRBailoutReason::NONE;
    }

    void OSRManager::buildOSRContext(JitContext& jitCtx,
                                      const OSRState& state,
                                      const bytecode::BytecodeProgram& program,
                                      vm::runtime::ExecutionContext& context,
                                      vm::runtime::VirtualMachine& vm,
                                      JitCodeCache& codeCache,
                                      value::Value* inputLocals,
                                      value::Value* outputLocals)
    {
        jitCtx = JitContext{};
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
        size_t sepPos = state.functionName.find("::");
        if (sepPos != std::string::npos)
            jitCtx.callingClassName = state.functionName.substr(0, sepPos);

        jitCtx.osrLocals = inputLocals;
        jitCtx.osrLocalCount = state.localCount;
        jitCtx.osrOutputLocals = outputLocals;
        jitCtx.osrExitOffset = 0;
        jitCtx.osrExited = false;
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

        JitContext jitCtx;
        buildOSRContext(jitCtx, state, program, context, vm, codeCache,
                        inputLocals.data(), outputLocals.data());

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

            // Loop function returned without setting osrExited
            return false;
        }
        catch (const OSRDeoptException& e)
        {
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
            lastResult.success = false;
            lastResult.deoptimized = true;
            lastResult.resumeIP = state.jumpBackOffset;

            lastResult.updatedLocals.clear();
            for (const auto& slot : state.locals)
            {
                lastResult.updatedLocals.push_back(slot.value);
            }
            writeBackState(lastResult, state, context);

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

        // MYT-153 Bug #2: also mirror writes into the SharedStackFrame, when
        // one is attached to the current call frame. VariableExecutor's
        // handleStoreLocal pre-populates this frame on every named local
        // store (speculative bookkeeping for potential lambda capture), and
        // handleLoadLocal reads from it *before* the stack — so skipping the
        // update here leaves post-OSR reads serving the captured pre-OSR
        // values. captureState already rejects `originatingLambda` frames
        // and frames with external sharedFrame owners, so any sharedFrame
        // reaching this point is the ordinary per-function bookkeeping copy.
        if (!context.callStack.empty() &&
            !context.callStack.back().originatingLambda &&
            context.callStack.back().sharedFrame)
        {
            auto sharedFrame = context.callStack.back().sharedFrame;
            for (size_t i = 0; i < result.updatedLocals.size() && i < state.localCount; ++i)
            {
                sharedFrame->setLocal(i, result.updatedLocals[i]);
            }
        }

        // Set instruction pointer: -1 because the interpreter loop increments
        context.instructionPointer = result.resumeIP - 1;
    }

    SlotType OSRManager::inferSlotType(const value::Value& val)
    {
        if (value::isInt(val)) return SlotType::INT;
        if (value::isFloat(val)) return SlotType::FLOAT;
        if (value::isBool(val)) return SlotType::BOOL;
        if (value::isString(val)) return SlotType::STRING;
        if (value::isInternedString(val)) return SlotType::STRING;
        if (value::isObject(val)) return SlotType::OBJECT;
        // MYT-208: distinct slot type so the JIT type tracker propagates the
        // STACK_OBJECT tag through type-quickened opcodes (LOAD_LOCAL_*).
        if (value::isStackObject(val)) return SlotType::STACK_OBJECT;
        if (value::isNativeArray(val)) return SlotType::ARRAY;
        return SlotType::BOXED;
    }
}

