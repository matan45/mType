#include "VirtualMachine.hpp"
#include <cstddef>
#include <cstdint>
#include "executors/StackOperationsExecutor.hpp"
#include "executors/ComparisonExecutor.hpp"
#include "executors/LogicalExecutor.hpp"
#include "executors/ArithmeticExecutor.hpp"
#include "executors/BitwiseExecutor.hpp"
#include "executors/ControlFlowExecutor.hpp"
#include "executors/VariableExecutor.hpp"
#include "executors/FunctionExecutor.hpp"
#include "executors/TypeExecutor.hpp"
#include "executors/ArrayExecutor.hpp"
#include "executors/ObjectExecutor.hpp"
#include "executors/LambdaExecutor.hpp"
#include "executors/ExceptionExecutor.hpp"
#include "executors/PrimitiveMethodExecutor.hpp"
#include "executors/InlineCacheExecutor.hpp"
#include "utils/ExceptionHandler.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/UserException.hpp"
#include "../../errors/SourceLocation.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../value/PromiseValue.hpp"
#include "../../value/ValueShim.hpp"
#include "../../debugger/DebugContext.hpp"
#include "../../debugger/DebugHookHelper.hpp"
#include "../profiler/ProfilerHookHelper.hpp"
#include "../profiler/ProfilerContext.hpp"
#include "../../gc/GC.hpp"
#include "../jit/OSRManager.hpp"
#include "../jit/ic/InlineCacheTable.hpp"
#include "../jit/ic/TypeFeedbackCollector.hpp"
#include <chrono>

namespace vm::runtime
{
    void VirtualMachine::ensureExecutors()
    {
        // Create (or recreate) the execution context as a heap-allocated member
        // so that executors holding a reference to it remain valid across calls.
        executionCtx = std::make_unique<ExecutionContext>(
            program, instructionPointer, callStack, maxCallStackSize,
            stackManager, stats, executionStart,
            debuggingEnabled, currentSourceFile, currentSourceLine);

        stackOpsExecutor = std::make_unique<StackOperationsExecutor>(*executionCtx);
        comparisonExecutor = std::make_unique<ComparisonExecutor>(*executionCtx);
        logicalExecutor = std::make_unique<LogicalExecutor>(*executionCtx);
        arithmeticExecutor = std::make_unique<ArithmeticExecutor>(*executionCtx);
        bitwiseExecutor = std::make_unique<BitwiseExecutor>(*executionCtx);
        controlFlowExecutor = std::make_unique<ControlFlowExecutor>(*executionCtx, this);
        if (jitEnabled && osrManager) {
            controlFlowExecutor->setOSRManager(osrManager.get());
        }
        variableExecutor = std::make_unique<VariableExecutor>(*executionCtx, environment);
        functionExecutor = std::make_unique<FunctionExecutor>(*executionCtx, environment, this);
        typeExecutor = std::make_unique<TypeExecutor>(*executionCtx, environment);
        arrayExecutor = std::make_unique<ArrayExecutor>(*executionCtx, environment);
        objectExecutor = std::make_unique<ObjectExecutor>(*executionCtx, environment, this);
        lambdaExecutor = std::make_unique<LambdaExecutor>(*executionCtx);
        exceptionExecutor = std::make_unique<ExceptionExecutor>(*executionCtx, this);
        primitiveMethodExecutor = std::make_unique<PrimitiveMethodExecutor>(*executionCtx, environment);

        // Set function executor reference in object executor for lambda-to-interface conversion
        objectExecutor->setFunctionExecutor(functionExecutor.get());

        // Phase 6: Initialize inline cache executor
        if (icEnabled && inlineCacheTable)
        {
            inlineCacheExecutor = std::make_unique<InlineCacheExecutor>(*executionCtx, this, *inlineCacheTable);
            inlineCacheExecutor->setObjectExecutor(objectExecutor.get());
            inlineCacheExecutor->setFunctionExecutor(functionExecutor.get());
        }

        // Initialize exception handler — pass executionCtx->program by reference so it
        // tracks cross-library program switches during exception handling
        exceptionHandler = std::make_unique<utils::ExceptionHandler>(
            executionCtx->program, stackManager, callStack, &loadedPrograms);
    }

    // VK-1378: Shared debug gate. interpretLoop caches the result of
    // isDebugActive() and refreshes it on the GC interval; the interop invoke
    // loops compute it once per call. Both then call debugPauseIfNeeded()
    // before each executeInstruction so breakpoints honour every execution
    // path, not just interpretLoop.
    bool VirtualMachine::isDebugActive() const
    {
        return debuggingEnabled && debugger::DebugHookHelper::isDebuggingEnabled();
    }

    void VirtualMachine::debugPauseIfNeeded()
    {
        auto* loc = program->getSourceLocation(instructionPointer);
        if (loc && loc->line > 0 && !loc->filename.empty())
        {
            errors::SourceLocation location(loc->filename,
                                            static_cast<int>(loc->line),
                                            static_cast<int>(loc->column));
            if (debugger::DebugHookHelper::shouldPause(location))
            {
                debugger::DebugHookHelper::waitForResume();
            }
        }
    }

    value::Value VirtualMachine::interpretLoop()
    {
        suspendedByAwait = false; // Reset flag at start

        // Initialize source location tracking by scanning forward for first instruction with location
        // This ensures error messages have correct location even before executing instructions
        if (callStack.empty() && instructionPointer == program->getEntryPoint())
        {
            for (size_t ip = instructionPointer; ip < program->getInstructionCount(); ++ip)
            {
                auto loc = program->getSourceLocation(ip);
                if (loc && !loc->filename.empty() && loc->line > 0)
                {
                    currentSourceFile = loc->filename;
                    currentSourceLine = static_cast<int>(loc->line);
                    break;
                }
            }
        }

        // Initialize executors (creates or refreshes the execution context)
        ensureExecutors();

        // GC: Counter for periodic collection checks
        size_t instructionsSinceGC = 0;

        // Cache profiler full mode flag for hot loop
        bool profilerFull = vm::profiler::ProfilerHookHelper::isProfilingEnabled()
                            && vm::profiler::ProfilerContext::getInstance().isFullMode();

        // Cache debug-active state. isDebuggingEnabled() routes through a
        // singleton + member call, so checking it per instruction isn't
        // free. Refresh inside the periodic GC block so a mid-run debug
        // attach still gets picked up within GC_CHECK_INTERVAL ops.
        bool debugActive = isDebugActive();

        // Use executionCtx->program for instruction fetch so cross-library calls
        // (which switch executionCtx->program to a library) fetch from the correct bytecode.
        auto& currentProgram = executionCtx->program;

        // MYT-??? Step 5: hoist the try/catch out of the per-iteration dispatch
        // loop. MSVC SEH has measurable per-try setup cost; on exception-free
        // code paths (the common case) we used to pay that cost per opcode.
        // Outer while re-enters the inner fast loop after a caught exception;
        // control-flow semantics (async rejected promises, finally suppression
        // of pending exceptions, debugger hooks) are preserved bit-identically.
        bool interpretDone = false;
        while (!interpretDone)
        {
            try
            {
                while (instructionPointer < currentProgram->getInstructionCount())
                {
                    // Check for pending rejection from an awaited promise.
                    // Set by the catch_ callback when a suspended task resumes
                    // after rejection.
                    if (pendingAwaitRejection.has_value())
                    {
                        auto rejection = std::move(pendingAwaitRejection.value());
                        pendingAwaitRejection.reset();
                        throw errors::UserException(
                            rejection.exceptionValue,
                            rejection.exceptionTypeName.empty() ? "RuntimeException" : rejection.exceptionTypeName
                        );
                    }

                    // GC: Periodic collection check. Also the cadence at which we
                    // refresh debugActive / profilerFull so a mid-run debug attach
                    // or profiler mode change takes effect within the interval.
                    if (++instructionsSinceGC >= gc::config::GC_CHECK_INTERVAL)
                    {
                        instructionsSinceGC = 0;
                        gc::GC::maybeCollect();
                        debugActive = isDebugActive();
                        profilerFull = vm::profiler::ProfilerHookHelper::isProfilingEnabled()
                                       && vm::profiler::ProfilerContext::getInstance().isFullMode();
                    }

                    const auto& instr = currentProgram->getInstruction(instructionPointer);

                    // Pending-exception suppression on RETURN inside a finally
                    // block (Java/C# semantics: a return inside finally
                    // suppresses the pending exception).
                    if (pendingException != nullptr &&
                        (instr.opcode == bytecode::OpCode::RETURN || instr.opcode == bytecode::OpCode::RETURN_VALUE) &&
                        currentFinallyOffset != SIZE_MAX &&
                        currentFinallyOffset == pendingFinallyOffset &&
                        instructionPointer > currentFinallyOffset)
                    {
                        pendingException.reset();
                        pendingFinallyOffset = SIZE_MAX;
                    }

                    // Debug hook: breakpoints / stepping. debugActive is
                    // cached + refreshed inside the GC block above.
                    if (debugActive)
                    {
                        debugPauseIfNeeded();
                    }

                    // Profiler: count opcode execution (full mode only)
                    if (profilerFull)
                    {
                        vm::profiler::ProfilerHookHelper::onOpcodeExecuted(static_cast<uint8_t>(instr.opcode));
                    }

                    // Execute — may throw UserException. Caught by the outer
                    // try; the inner loop body pays no per-iter SEH setup.
                    executeInstruction(instr);

                    stats.instructionsExecuted++;

                    if (suspendedByAwait)
                    {
                        suspendedByAwait = false;
                        interpretDone = true;
                        break;
                    }

                    if (instr.opcode == bytecode::OpCode::HALT)
                    {
                        interpretDone = true;
                        break;
                    }

                    instructionPointer++;
                }
                // Inner loop exited without setting interpretDone → normal
                // completion (IP reached end). End the outer loop too.
                if (!interpretDone) interpretDone = true;
            }
            catch (errors::UserException& e)
            {
                // Delegate exception handling to ExceptionHandler.
                auto result = exceptionHandler->handleUserException(e, instructionPointer, currentFinallyOffset);

                // Debug hook: break on thrown exception.
                if (debugActive)
                {
                    bool isUncaught = !result.handled;
                    auto* exLoc = program->getSourceLocation(instructionPointer);
                    errors::SourceLocation exLocation = exLoc
                        ? errors::SourceLocation(exLoc->filename, static_cast<int>(exLoc->line), static_cast<int>(exLoc->column))
                        : errors::SourceLocation(currentSourceFile, currentSourceLine, 0);
                    debugger::DebugHookHelper::handleException(
                        e.getExceptionTypeName(),
                        e.what(),
                        exLocation,
                        isUncaught);
                }

                if (!result.handled)
                {
                    // No matching catch. Check for async-function rejected-
                    // promise propagation — the async body swallows the
                    // exception and returns a rejected promise.
                    bool asyncRejected = false;
                    if (!callStack.empty())
                    {
                        const CallFrame& currentFrame = callStack.back();
                        auto funcMetadata = program->getFunctionMeta(currentFrame.functionName);

                        if (funcMetadata && funcMetadata->isAsync)
                        {
                            std::string errorMsg = e.getExceptionTypeName();
                            if (value::isObject(e.getExceptionValue()))
                            {
                                auto objInstance = value::asObject(e.getExceptionValue());
                                if (objInstance)
                                {
                                    try
                                    {
                                        value::Value msgValue = objInstance->getFieldValue("msg");
                                        // MYT-317: SSO-aware error-message extraction.
                                        if (value::isAnyString(msgValue))
                                        {
                                            errorMsg += ": ";
                                            errorMsg += value::asStringView(msgValue);
                                        }
                                    }
                                    catch (...)
                                    {
                                        try
                                        {
                                            value::Value messageValue = objInstance->getFieldValue("message");
                                            if (value::isAnyString(messageValue))
                                            {
                                                errorMsg += ": ";
                                                errorMsg += value::asStringView(messageValue);
                                            }
                                        }
                                        catch (...)
                                        {
                                            // Neither field exists; keep the type name
                                        }
                                    }
                                }
                            }

                            auto rejectedPromise = std::make_shared<value::PromiseValue>();
                            rejectedPromise->rejectWithException(e.getExceptionValue(), e.getExceptionTypeName(), errorMsg);

                            controlFlowExecutor->handleReturn();
                            stackManager->push(std::static_pointer_cast<value::PromiseValue>(rejectedPromise));

                            instructionPointer++;
                            stats.instructionsExecuted++;
                            asyncRejected = true;
                        }
                    }

                    if (!asyncRejected)
                    {
                        // Not in an async function, or no call stack — escape.
                        throw;
                    }
                    // Async-rejection path: fall through, outer while re-enters.
                }
                else
                {
                    // Handled: jump to catch/finally; record pending exception
                    // if we went to finally.
                    instructionPointer = result.newInstructionPointer;
                    if (result.jumpedToFinally)
                    {
                        pendingException = std::make_unique<errors::UserException>(e);
                        pendingFinallyOffset = result.newInstructionPointer;
                    }
                    else
                    {
                        pendingException.reset();
                        pendingFinallyOffset = SIZE_MAX;
                    }
                    stats.instructionsExecuted++;
                }
                // Fall through: outer while re-enters the inner fast loop
                // with updated instructionPointer / pending state.
            }
        }

        // Calculate execution time
        auto executionEnd = std::chrono::steady_clock::now();
        stats.executionTime = std::chrono::duration_cast<std::chrono::microseconds>(
            executionEnd - executionStart);

        // Return top of stack or void
        if (!stackManager->empty())
        {
            return stackManager->pop();
        }
        return std::monostate{};
    }
}
