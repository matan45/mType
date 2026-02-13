#include "DeoptimizationHandler.hpp"

namespace vm::jit::guards
{
    bool DeoptimizationHandler::handleDeopt(const DeoptState& deoptState,
                                             const OSRState& osrState,
                                             vm::runtime::ExecutionContext& context)
    {
        if (context.callStack.empty())
        {
            return false;
        }

        size_t localBase = context.callStack.back().localBase;

        // Write locals back to interpreter stack
        for (size_t i = 0; i < deoptState.locals.size() && i < osrState.localCount; ++i)
        {
            size_t stackIdx = localBase + i;
            if (stackIdx < context.stackManager->size())
            {
                context.stackManager->getStack()[stackIdx] = deoptState.locals[i];
            }
        }

        // Set instruction pointer to the deoptimization bytecode offset
        // Subtract 1 because the interpreter loop will increment IP
        context.instructionPointer = deoptState.bytecodeOffset - 1;

        return true;
    }
}
