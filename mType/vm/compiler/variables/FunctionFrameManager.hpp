#pragma once
#include <string>
#include <vector>
#include <cstddef>

namespace vm::compiler::variables
{
    /**
     * Manages function call frames for proper local variable scoping
     * Tracks parameters, return types, and local variable slots per function
     */
    class FunctionFrameManager
    {
    public:
        struct FunctionFrame {
            size_t localStartSlot;
            int scopeDepthStart;
            std::string returnType;
            bool isLambda = false;  // Track if this frame is for a lambda
            bool isAsync = false;   // Track if this frame is for an async function/lambda
            size_t maxLocalSlot = 0;  // Track the maximum local slot used in this function
        };

        FunctionFrameManager() = default;
        ~FunctionFrameManager() = default;

        // Frame management
        void enterFunctionFrame(const std::string& returnType, size_t localStartSlot,
                               int scopeDepthStart, bool isLambda = false, bool isAsync = false);
        void exitFunctionFrame();

        // Frame information
        bool isInFunction() const;
        const FunctionFrame& currentFrame() const;
        FunctionFrame& currentFrame();

        // Return type checking
        std::string getReturnType() const;

        // Local count calculation
        size_t getLocalCount() const;

        // Update max local slot
        void updateMaxLocalSlot(size_t slot);

        // Check if in lambda
        bool isInLambda() const;

    private:
        std::vector<FunctionFrame> functionFrameStack;
    };
}
