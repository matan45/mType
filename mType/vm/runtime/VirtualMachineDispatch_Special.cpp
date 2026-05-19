#include "VirtualMachine.hpp"
#include <cstddef>
#include "executors/ExceptionExecutor.hpp"
#include "executors/LambdaExecutor.hpp"
#include "executors/TypeExecutor.hpp"
#include "../../debugger/DebugContext.hpp"
#include "../../debugger/DebugHookHelper.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/UserException.hpp"
#include "../../value/AsyncPromiseValue.hpp"
#include "../../value/PromiseValue.hpp"
#include "../../value/ValueShim.hpp"
#include "../jit/JitCodeCache.hpp"
#include "../jit/JitCompiler.hpp"
#include "../jit/JitProfiler.hpp"

namespace vm::runtime
{
    void VirtualMachine::dispatchSpecialOps(const bytecode::BytecodeProgram::Instruction& instr)
    {
        using OpCode = bytecode::OpCode;

        switch (instr.opcode)
        {
        // Types
        case OpCode::INSTANCEOF: typeExecutor->handleInstanceof(instr);
            break;
        case OpCode::INSTANCEOF_TYPEPARAM: typeExecutor->handleInstanceofTypeParam(instr);
            break;
        case OpCode::CAST: typeExecutor->handleCast(instr);
            break;
        case OpCode::CAST_TYPEPARAM: typeExecutor->handleCastTypeParam(instr);
            break;
        case OpCode::BIND_TYPE_ARGS: typeExecutor->handleBindTypeArgs(instr);
            break;

        // Lambda
        case OpCode::LAMBDA: lambdaExecutor->handleLambda(instr);
            break;

        // Exception handling
        case OpCode::TRY_BEGIN:
            // Pending exception from a prior finally: re-throw before entering
            // the new try block.
            if (pendingException != nullptr)
            {
                errors::UserException exToRethrow = *pendingException;
                pendingException.reset();
                pendingFinallyOffset = SIZE_MAX;
                currentFinallyOffset = SIZE_MAX;
                throw exToRethrow;
            }
            currentFinallyOffset = SIZE_MAX;
            exceptionExecutor->handleTryBegin(instr);
            break;
        case OpCode::TRY_END:
            // TRY_END marks end of try body OR end of entire try-catch-finally
            // construct. A pending exception here means we've exited the
            // finally block — re-throw.
            if (pendingException != nullptr)
            {
                errors::UserException exToRethrow = *pendingException;
                pendingException.reset();
                pendingFinallyOffset = SIZE_MAX;
                currentFinallyOffset = SIZE_MAX;
                throw exToRethrow;
            }
            currentFinallyOffset = SIZE_MAX;
            exceptionExecutor->handleTryEnd(instr);
            break;
        case OpCode::CATCH: exceptionExecutor->handleCatch(instr);
            break;
        case OpCode::THROW: exceptionExecutor->handleThrow(instr);
            break;
        case OpCode::FINALLY:
            currentFinallyOffset = instructionPointer;
            exceptionExecutor->handleFinally(instr);
            break;

        // Async/await
        case OpCode::CREATE_PROMISE:
            {
                // Wrap TOS in a Promise. Raw INT collapses to inline PROMISE_INT
                // (no heap allocation); other inputs allocate AsyncPromiseValue
                // for full event-loop / callback support.
                value::Value val = stackManager->pop();
                if (value::isInt(val))
                {
                    stackManager->push(value::Value(value::asInt(val), value::Value::PromiseIntTag{}));
                }
                else
                {
                    auto promise = std::make_shared<value::AsyncPromiseValue>(val);
                    stackManager->push(std::shared_ptr<value::PromiseValue>(promise));
                }
                break;
            }
        case OpCode::AWAIT:
            executeAwait();
            break;
        case OpCode::PROMISE_RESOLVE:
            // Reserved for future use — no emitter generates this opcode.
            break;

        // Debug
        case OpCode::BREAKPOINT:
            if (debuggingEnabled && debugger::DebugHookHelper::isDebuggingEnabled())
            {
                auto sourceLoc = program->getSourceLocation(instructionPointer);
                if (sourceLoc)
                {
                    errors::SourceLocation location(sourceLoc->filename,
                                                    static_cast<int>(sourceLoc->line),
                                                    static_cast<int>(sourceLoc->column));

                    if (debugger::DebugHookHelper::shouldPause(location))
                    {
                        debugger::DebugHookHelper::waitForResume();
                    }
                }
            }
            break;
        case OpCode::LINE:
            if (instr.hasOperands())
            {
                currentSourceLine = static_cast<int>(instr.inlineOperands[0]);
            }
            break;
        case OpCode::SOURCE_FILE:
            if (instr.hasOperands())
            {
                size_t stringIndex = instr.inlineOperands[0];
                const auto& constPool = program->getConstantPool();
                if (stringIndex < constPool.strings.size())
                {
                    currentSourceFile = constPool.getString(stringIndex);
                }
            }
            break;

        // Profiling
        case OpCode::PROFILE_ENTER:
            if (jitEnabled && jitProfiler && !callStack.empty())
            {
                // MYT-197: resolve the frame's handle to the owning program's
                // interned name. JitProfiler + JitCompiler still key on
                // std::string (cold path — PROFILE_ENTER only fires while
                // tiering up).
                const auto& frame = callStack.back();
                const bytecode::BytecodeProgram* framePrg =
                    (frame.programIndex < loadedPrograms.size())
                        ? loadedPrograms[frame.programIndex]
                        : program;
                const std::string& funcName = framePrg->getFrameName(frame.functionName);
                bool justBecameHot = jitProfiler->recordEntry(funcName);
                if (justBecameHot && jitCompiler && jitCodeCache)
                {
                    // MYT-314: compile against framePrg (the program owning
                    // this frame), not the top-level `program`. JitCompiler::
                    // compile does program.getFunction(funcName), which would
                    // silently bail out for imported functions if we passed
                    // the top-level program.
                    jitCompiler->compile(funcName, *framePrg, *jitCodeCache, typeFeedbackCollector.get());
                }
            }
            break;
        case OpCode::PROFILE_EXIT:
            // Profiling uses entry counts only.
            break;

        default:
            throw errors::RuntimeException("Unimplemented opcode: " +
                std::string(bytecode::getOpCodeName(instr.opcode)));
        }
    }
}
