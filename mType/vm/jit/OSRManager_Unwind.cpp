#include "OSRManager.hpp"
#include <cstddef>
#include <iostream>
#include <typeinfo>
#include "JitHelpers.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../../value/ValueShim.hpp"

namespace vm::jit
{
    bool OSRManager::executeOSRLoop(OSRLoopFunction func,
                                     const OSRState& state,
                                     const bytecode::BytecodeProgram& program,
                                     vm::runtime::ExecutionContext& context,
                                     vm::runtime::VirtualMachine& vm,
                                     JitCodeCache& codeCache)
    {
        std::vector<value::Value> inputLocals(state.localCount);
        std::vector<value::Value> outputLocals(state.localCount);

        for (const auto& slotInfo : state.locals)
        {
            if (slotInfo.slot < state.localCount)
            {
                inputLocals[slotInfo.slot] = slotInfo.value;
            }
        }

        // Initialize output locals to same values (for deopt safety).
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

            // MYT-254: any std::exception thrown by a JIT helper (e.g. a
            // missing receiver-kind branch in jit_call_method) is caught at
            // the helper's site and stored on the JIT context, since
            // exceptions can't unwind through asmjit-generated frames. The
            // emitted OSR loop checks ctx->pendingException at every back
            // edge (see emitOSRCodegenLoop) and falls into the osrExit
            // handler the moment one is observed, so by the time func()
            // returns to us pendingException is the authoritative signal.
            // Rethrow BEFORE inspecting osrExited / writing back updatedLocals
            // so the catch (std::exception&) below restores the pre-OSR
            // locals and the interpreter resumes the iteration that threw.
            if (jitCtx.pendingException)
            {
                std::rethrow_exception(jitCtx.pendingException);
            }

            if (jitCtx.osrExited)
            {
                lastResult.success = true;
                lastResult.deoptimized = false;
                lastResult.resumeIP = jitCtx.osrExitOffset;
                lastResult.updatedLocals = std::move(outputLocals);
                writeBackState(lastResult, state, context);
                return true;
            }

            // Loop function returned without setting osrExited.
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
        catch (const std::exception& e)
        {
            // MYT-248/249/250: surface the typed exception name + message
            // before rolling back state and rethrowing. Without this an
            // exception thrown by a native helper (use-after-free, bad alloc,
            // logic_error from MethodResolver, etc.) bubbles up through the
            // VM loop's UserException-only catch and is silently swallowed
            // when Main.cpp's std::exception catch can't classify it as a
            // script error.
            std::cerr << "[OSR] caught std::exception during JIT loop execution: "
                      << typeid(e).name() << ": " << e.what()
                      << " (jumpBackOffset=" << state.jumpBackOffset << ")\n";
            std::cerr.flush();

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
        catch (...)
        {
            // MYT-248/249/250: silent-zero-output failure mode for the three
            // benchmarks — a non-std exception (or SEH if MSVC is configured
            // to translate them) escapes from JIT-emitted code or the IC
            // populate path. Make it visible so the next repro is not silent.
            std::cerr << "[OSR] caught unknown (non-std) exception during JIT "
                      << "loop execution (jumpBackOffset=" << state.jumpBackOffset
                      << "). Likely culprit: JIT helper returning into freed memory, "
                      << "stale IC entry, or SEH from JIT-emitted code.\n";
            std::cerr.flush();

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

        // -1 because the interpreter loop increments after the OSR return.
        context.instructionPointer = result.resumeIP - 1;
    }
}
