#include "FunctionFrameManager.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace vm::compiler::variables
{
    void FunctionFrameManager::enterFunctionFrame(const std::string& returnType, size_t localStartSlot,
                                                  int scopeDepthStart, bool isLambda)
    {
        FunctionFrame frame;
        frame.localStartSlot = localStartSlot;
        frame.scopeDepthStart = scopeDepthStart;
        frame.returnType = returnType;
        frame.isLambda = isLambda;
        frame.maxLocalSlot = localStartSlot;
        functionFrameStack.push_back(frame);
    }

    void FunctionFrameManager::exitFunctionFrame()
    {
        if (functionFrameStack.empty()) {
            throw errors::RuntimeException("Function frame stack underflow");
        }
        functionFrameStack.pop_back();
    }

    bool FunctionFrameManager::isInFunction() const
    {
        return !functionFrameStack.empty();
    }

    const FunctionFrameManager::FunctionFrame& FunctionFrameManager::currentFrame() const
    {
        if (functionFrameStack.empty()) {
            throw errors::RuntimeException("Not in a function context");
        }
        return functionFrameStack.back();
    }

    FunctionFrameManager::FunctionFrame& FunctionFrameManager::currentFrame()
    {
        if (functionFrameStack.empty()) {
            throw errors::RuntimeException("Not in a function context");
        }
        return functionFrameStack.back();
    }

    std::string FunctionFrameManager::getReturnType() const
    {
        if (functionFrameStack.empty()) {
            return "";
        }
        return functionFrameStack.back().returnType;
    }

    size_t FunctionFrameManager::getLocalCount() const
    {
        if (functionFrameStack.empty()) {
            return 0;
        }
        // Use maxLocalSlot instead of nextLocalSlot to account for variables that were popped by endScope
        return functionFrameStack.back().maxLocalSlot - functionFrameStack.back().localStartSlot;
    }

    void FunctionFrameManager::updateMaxLocalSlot(size_t slot)
    {
        if (!functionFrameStack.empty() && slot > functionFrameStack.back().maxLocalSlot) {
            functionFrameStack.back().maxLocalSlot = slot;
        }
    }

    bool FunctionFrameManager::isInLambda() const
    {
        return !functionFrameStack.empty() && functionFrameStack.back().isLambda;
    }
}
