#include "ControlFlowExecutor.hpp"
#include "../../../value/PromiseValue.hpp"
#include "../../../value/AsyncPromiseValue.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../profiler/ProfilerHookHelper.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../jit/OSRManager.hpp"
#include "../VirtualMachine.hpp"

namespace vm::runtime
{
    // MYT-319: handleJumpBack, handleReturn, handleReturnValue stay here
    // because they pull in OSR / debug / profiler / Promise / ObjectInstance.
    // Pure jump variants live in the header for dispatch inlining.

    void ControlFlowExecutor::handleJumpBack(const bytecode::BytecodeProgram::Instruction& instr) {
        // Phase 5: OSR check at loop back-edge
        if (osrManager && context.vm && context.vm->isJitEnabled())
        {
            auto* compiler = context.vm->getJitCompiler();
            auto* codeCache = context.vm->getJitCodeCache();
            if (compiler && codeCache)
            {
                if (osrManager->tryOSR(context.instructionPointer,
                                        *context.program,
                                        context,
                                        *context.vm,
                                        *compiler,
                                        *codeCache))
                {
                    // OSR executed the loop natively and set instructionPointer
                    return;
                }
            }
        }

        // Normal interpreter path: jump back to loop start
        context.instructionPointer = instr.inlineOperands[0] - 1;  // -1 because loop increments
    }

    void ControlFlowExecutor::handleReturn() {
        if (context.callStack.empty()) {
            // Top-level return - halt execution
            context.instructionPointer = context.program->getInstructionCount();
        } else {
            // CallFrame is move-only (TypeArgMapPtr); move-out from the
            // back, then pop. The moved-from slot is destroyed by pop_back.
            CallFrame frame = std::move(context.callStack.back());

            // MYT-197: resolve the frame's interned handle to a string only
            // when the gated profiler / debugger hook actually fires. Both
            // gates short-circuit when disabled, keeping the hot path free
            // of std::string materialisation.
            if (vm::profiler::ProfilerHookHelper::isProfilingEnabled()) {
                vm::profiler::ProfilerHookHelper::onFunctionExit(context.frameName(frame));
            }

            // Notify debugger of function exit BEFORE popping the call stack
            if (debugger::DebugHookHelper::isDebuggingEnabled()) {
                debugger::DebugHookHelper::exitFunctionHook(context.frameName(frame));
            }

            // MYT-208: release stack-promoted allocations BEFORE pop. Normal
            // RETURN is the dominant path; releasing here is what actually
            // recycles pool slots on the hot path.
            context.callStack.back().releaseStackObjects();
            context.callStack.pop_back();
            context.instructionPointer = frame.returnAddress;

            // Restore caller's program if returning across library boundary
            if (!context.callStack.empty() && context.loadedPrograms) {
                const auto& callerFrame = context.callStack.back();
                if (callerFrame.programIndex < context.loadedPrograms->size()) {
                    context.program = (*context.loadedPrograms)[callerFrame.programIndex];
                }
            }

            // Restore stack to the caller's frame base in a single resize().
            // Bulk truncation avoids per-element pop_back() overhead on the
            // hot return path (~2.76M returns in recursive.mt).
            if (context.stackManager->size() > frame.frameBase) {
                context.stackManager->getStack().resize(frame.frameBase);
            }
        }
    }

    void ControlFlowExecutor::handleReturnValue() {
        value::Value returnVal = context.stackManager->pop();

        // Check if we're returning from an async function
        // If so, wrap the return value in a PromiseValue (unless already wrapped by CREATE_PROMISE)
        if (!context.callStack.empty()) {
            const CallFrame& frame = context.callStack.back();
            // MYT-197: O(1) handle-keyed metadata lookup.
            auto funcMetadata = context.program->getFunctionMeta(frame.functionName);

            if (funcMetadata && funcMetadata->isAsync) {
                // Check if already wrapped in Promise (by CREATE_PROMISE opcode
                // or its fused variants, which may emit the inline PROMISE_INT
                // form). isAnyPromise covers both heap and inline shapes — the
                // inline form must NOT be re-wrapped, that would defeat the
                // point of the resolved-fast-path.
                if (!value::isAnyPromise(returnVal)) {
                    // Wrap return value in AsyncPromiseValue for async functions
                    // Use AsyncPromiseValue to support event loop and callbacks
                    auto promise = std::make_shared<value::AsyncPromiseValue>(returnVal);
                    returnVal = std::shared_ptr<value::PromiseValue>(promise);
                }
            }
        }

        handleReturn();
        context.stackManager->push(returnVal);
    }
}
