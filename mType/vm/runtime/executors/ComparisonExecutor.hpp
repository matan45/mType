#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace vm::runtime
{
    /**
     * Executes comparison opcodes
     * Handles EQ, NE, LT, GT, LE, GE operations
     */
    class ComparisonExecutor
    {
    public:
        explicit ComparisonExecutor(ExecutionContext& ctx);
        ~ComparisonExecutor() = default;

        // Comparison operations
        void handleEq();
        void handleNe();
        void handleLt();
        void handleGt();
        void handleLe();
        void handleGe();

    private:
        ExecutionContext& context;
    };
}
