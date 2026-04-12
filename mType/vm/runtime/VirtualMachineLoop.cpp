#include "VirtualMachine.hpp"
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
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../value/PromiseValue.hpp"
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
            environment, stackManager, stats, executionStart,
            debuggingEnabled, currentSourceFile, currentSourceLine, this);

        stackOpsExecutor = std::make_unique<StackOperationsExecutor>(*executionCtx);
        comparisonExecutor = std::make_unique<ComparisonExecutor>(*executionCtx);
        logicalExecutor = std::make_unique<LogicalExecutor>(*executionCtx);
        arithmeticExecutor = std::make_unique<ArithmeticExecutor>(*executionCtx);
        bitwiseExecutor = std::make_unique<BitwiseExecutor>(*executionCtx);
        controlFlowExecutor = std::make_unique<ControlFlowExecutor>(*executionCtx);
        if (jitEnabled && osrManager) {
            controlFlowExecutor->setOSRManager(osrManager.get());
        }
        variableExecutor = std::make_unique<VariableExecutor>(*executionCtx);
        functionExecutor = std::make_unique<FunctionExecutor>(*executionCtx);
        typeExecutor = std::make_unique<TypeExecutor>(*executionCtx);
        arrayExecutor = std::make_unique<ArrayExecutor>(*executionCtx);
        objectExecutor = std::make_unique<ObjectExecutor>(*executionCtx);
        lambdaExecutor = std::make_unique<LambdaExecutor>(*executionCtx);
        exceptionExecutor = std::make_unique<ExceptionExecutor>(*executionCtx);
        primitiveMethodExecutor = std::make_unique<PrimitiveMethodExecutor>(*executionCtx);

        // Set function executor reference in object executor for lambda-to-interface conversion
        objectExecutor->setFunctionExecutor(functionExecutor.get());

        // Phase 6: Initialize inline cache executor
        if (icEnabled && inlineCacheTable)
        {
            inlineCacheExecutor = std::make_unique<InlineCacheExecutor>(*executionCtx, *inlineCacheTable);
            inlineCacheExecutor->setObjectExecutor(objectExecutor.get());
            inlineCacheExecutor->setFunctionExecutor(functionExecutor.get());
        }

        // Initialize exception handler
        exceptionHandler = std::make_unique<utils::ExceptionHandler>(program, stackManager, callStack);
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

        while (instructionPointer < program->getInstructionCount())
        {
            // Check for pending rejection from an awaited promise
            // This is set by the catch_ callback when a suspended task resumes after rejection
            if (pendingAwaitRejection.has_value())
            {
                auto rejection = std::move(pendingAwaitRejection.value());
                pendingAwaitRejection.reset();
                throw errors::UserException(
                    rejection.exceptionValue,
                    rejection.exceptionTypeName.empty() ? "RuntimeException" : rejection.exceptionTypeName
                );
            }

            // GC: Periodic collection check
            if (++instructionsSinceGC >= gc::config::GC_CHECK_INTERVAL)
            {
                instructionsSinceGC = 0;
                gc::GC::maybeCollect();
            }

            const auto& instr = program->getInstruction(instructionPointer);

            // Check if we have a pending exception and we're hitting RETURN
            // According to Java/C# semantics: a return in finally SUPPRESSES the pending exception
            // The function returns normally and the exception is discarded
            // IMPORTANT: Only suppress if:
            // 1. We're inside a finally block (currentFinallyOffset is set)
            // 2. The IP is between FINALLY and TRY_END (we're executing finally body)
            // We can check #2 by seeing if IP > currentFinallyOffset (after FINALLY instruction)
            if (pendingException != nullptr &&
                (instr.opcode == bytecode::OpCode::RETURN || instr.opcode == bytecode::OpCode::RETURN_VALUE) &&
                currentFinallyOffset != SIZE_MAX &&
                currentFinallyOffset == pendingFinallyOffset &&
                instructionPointer > currentFinallyOffset)
            {
                // Clear the pending exception - the return inside finally overrides it
                pendingException.reset();
                pendingFinallyOffset = SIZE_MAX;
                // Continue with normal return (do NOT re-throw)
            }

            // PERFORMANCE: Source location is tracked via LINE and SOURCE_FILE opcodes
            // No per-instruction lookup needed - saves hash map lookup on every instruction

            // Debug hook: Check for breakpoints and stepping before executing instruction
            // Only do expensive checks when debugging is actually enabled
            if (debuggingEnabled && debugger::DebugHookHelper::isDebuggingEnabled())
            {
                // Look up source location from the bytecode program's source map
                // (LINE/SOURCE_FILE opcodes are not emitted; use the compiler's map instead)
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

            // Profiler: count opcode execution (full mode only)
            if (profilerFull)
            {
                vm::profiler::ProfilerHookHelper::onOpcodeExecuted(static_cast<uint8_t>(instr.opcode));
            }

            try
            {
                // Execute instruction
                executeInstruction(instr);
            }
            catch (errors::UserException& e)
            {
                // Delegate exception handling to ExceptionHandler
                auto result = exceptionHandler->handleUserException(e, instructionPointer, currentFinallyOffset);

                // Debug hook: check if debugger wants to break on this exception
                if (debuggingEnabled && debugger::DebugHookHelper::isDebuggingEnabled())
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
                    // No matching catch found - check if we're in an async function
                    if (!callStack.empty())
                    {
                        const CallFrame& currentFrame = callStack.back();
                        auto funcMetadata = program->getFunction(currentFrame.functionName);

                        if (funcMetadata && funcMetadata->isAsync)
                        {
                            // We're in an async function - don't propagate exception
                            // Instead, create a rejected promise and return it

                            // Build error message from exception
                            std::string errorMsg = e.getExceptionTypeName();
                            if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(e.getExceptionValue()))
                            {
                                auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(e.getExceptionValue());
                                if (objInstance)
                                {
                                    // Try to get error message from common exception fields
                                    try
                                    {
                                        value::Value msgValue = objInstance->getFieldValue("msg");
                                        if (std::holds_alternative<std::string>(msgValue))
                                        {
                                            errorMsg += ": " + std::get<std::string>(msgValue);
                                        }
                                    }
                                    catch (...)
                                    {
                                        // Field doesn't exist, try "message" field
                                        try
                                        {
                                            value::Value messageValue = objInstance->getFieldValue("message");
                                            if (std::holds_alternative<std::string>(messageValue))
                                            {
                                                errorMsg += ": " + std::get<std::string>(messageValue);
                                            }
                                        }
                                        catch (...)
                                        {
                                            // Neither field exists, use just the type name
                                        }
                                    }
                                }
                            }

                            // Create a rejected promise with the full exception object
                            auto rejectedPromise = std::make_shared<value::PromiseValue>();
                            rejectedPromise->rejectWithException(e.getExceptionValue(), e.getExceptionTypeName(), errorMsg);

                            // Clean up call frame and return the rejected promise
                            controlFlowExecutor->handleReturn();
                            stackManager->push(std::static_pointer_cast<value::PromiseValue>(rejectedPromise));

                            // Move past the CALL instruction (handleReturn set IP to CALL position)
                            // We must increment here because continue skips the normal IP increment
                            instructionPointer++;
                            stats.instructionsExecuted++;
                            continue; // Continue execution with the rejected promise on stack
                        }
                    }

                    // Not in an async function, or no call stack - re-throw
                    throw;
                }

                // Jump to the catch/finally block
                instructionPointer = result.newInstructionPointer;

                // If we jumped to a finally block (not a catch), store the exception as pending
                // It needs to be re-thrown after the finally block completes
                if (result.jumpedToFinally)
                {
                    pendingException = std::make_unique<errors::UserException>(e);
                    pendingFinallyOffset = result.newInstructionPointer;  // Store which finally has the pending exception
                }
                else
                {
                    // Jumped to a catch block - exception is caught, clear any pending exception
                    pendingException.reset();
                    pendingFinallyOffset = SIZE_MAX;
                }

                // Continue execution from the catch/finally block
                // Don't increment IP here - the normal loop will handle it
                stats.instructionsExecuted++;
                continue; // Skip the rest of the loop iteration
            }

            stats.instructionsExecuted++;

            // Check if we suspended due to AWAIT - if so, break without incrementing IP
            if (suspendedByAwait)
            {
                suspendedByAwait = false; // Reset flag
                break; // Exit loop without incrementing IP (already done by AWAIT handler)
            }

            // Check for halt
            if (instr.opcode == bytecode::OpCode::HALT)
            {
                break;
            }

            instructionPointer++;
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
