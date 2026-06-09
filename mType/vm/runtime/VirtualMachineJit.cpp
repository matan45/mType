#include "VirtualMachine.hpp"
#include <cstddef>
#include "../../errors/RuntimeException.hpp"
#include "../../errors/UserException.hpp"
#include "utils/ExceptionHandler.hpp"
#include "executors/ArrayExecutor.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../bytecode/OpCode.hpp"
#include "../../environment/Environment.hpp"
#include "../../environment/registry/ClassRegistry.hpp"

namespace vm::runtime
{
    void VirtualMachine::restoreJitMiniInterpretState(
        size_t savedIP,
        size_t savedCallStackDepth,
        size_t savedStackSize,
        const bytecode::BytecodeProgram* savedProgram,
        bool switchedProgram)
    {
        while (callStack.size() > savedCallStackDepth)
        {
            // MYT-208: release stack-promoted allocations before pop.
            callStack.back().releaseStackObjects();
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
    }

    bool VirtualMachine::tryDispatchInScopeJitUserException(
        errors::UserException& e,
        size_t savedCallStackDepth,
        const bytecode::BytecodeProgram* jitCurrentProgram)
    {
        // MYT-254: only dispatch when the top frame has a covering handler
        // entry. handleUserException can pop frames as it searches; without
        // this gate we could lose the mini-interpret entry frame and turn a
        // handlable in-function throw into an uncaught escape
        // (try_catch_finally_hot regression).
        bool dispatchInScope = false;
        if (!callStack.empty() && callStack.size() > savedCallStackDepth)
        {
            auto* funcMeta = jitCurrentProgram->getFunctionMeta(
                callStack.back().functionName);
            if (funcMeta && funcMeta->exceptionTable.size() > 0)
            {
                const auto* handler = funcMeta->exceptionTable.findHandler(
                    instructionPointer,
                    e.getExceptionTypeName(),
                    e.getExceptionValue());
                if (handler) dispatchInScope = true;
            }
        }
        if (!dispatchInScope || !exceptionHandler) return false;

        auto result = exceptionHandler->handleUserException(
            e, instructionPointer, currentFinallyOffset);
        if (!result.handled || callStack.size() <= savedCallStackDepth)
            return false;

        instructionPointer = result.newInstructionPointer;
        if (result.jumpedToFinally)
        {
            pendingException = std::make_unique<errors::UserException>(e);
        }
        return true;
    }

    value::Value VirtualMachine::runJitMiniInterpret(
        size_t savedIP,
        size_t savedCallStackDepth,
        size_t savedStackSize,
        const bytecode::BytecodeProgram* savedProgram,
        bool switchedProgram)
    {
        // This function runs its own interpreter loop on the native C++
        // stack so it contributes to native recursion depth. Caller has
        // already pushed the frame and set instructionPointer.
        ++jitNativeDepth;

        // Use executionCtx->program so cross-library calls fetch from the
        // correct bytecode. Re-read each iteration via the reference so an
        // executor that switches program (e.g. cross-library CALL) is
        // honoured by the next instruction fetch.
        auto& jitCurrentProgram = executionCtx->program;
        // VK-1378: keep breakpoints working on this JIT-invoked mini-interpreter
        // too, so the debug hook is uniform across every bytecode-driving loop.
        bool debugActive = isDebugActive();
        while (callStack.size() > savedCallStackDepth)
        {
            if (instructionPointer >= jitCurrentProgram->getInstructionCount())
            {
                break;
            }

            const auto& instr = jitCurrentProgram->getInstruction(instructionPointer);

            if (debugActive)
            {
                debugPauseIfNeeded();
            }

            try
            {
                executeInstruction(instr);
            }
            catch (errors::UserException& e)
            {
                if (tryDispatchInScopeJitUserException(
                        e, savedCallStackDepth, jitCurrentProgram))
                {
                    continue;
                }
                --jitNativeDepth;
                restoreJitMiniInterpretState(savedIP, savedCallStackDepth,
                                              savedStackSize, savedProgram,
                                              switchedProgram);
                throw;
            }
            catch (...)
            {
                --jitNativeDepth;
                restoreJitMiniInterpretState(savedIP, savedCallStackDepth,
                                              savedStackSize, savedProgram,
                                              switchedProgram);
                throw;
            }

            instructionPointer++;
        }

        --jitNativeDepth;

        value::Value result = std::monostate{};
        if (stackManager->size() > savedStackSize)
        {
            result = stackManager->pop();
        }

        while (stackManager->size() > savedStackSize)
        {
            stackManager->pop();
        }

        instructionPointer = savedIP;

        // MYT-182: restore the pre-call program. The mini-interpret loop
        // exits on frame-depth check rather than running a RETURN handler,
        // so restore explicitly here.
        if (switchedProgram)
        {
            executionCtx->program = savedProgram;
        }

        return result;
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
