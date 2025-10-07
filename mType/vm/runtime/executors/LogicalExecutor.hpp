#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace vm::runtime
{
    /**
     * Executes logical opcodes
     * Handles AND, OR, NOT operations
     */
    class LogicalExecutor
    {
    public:
        explicit LogicalExecutor(ExecutionContext& ctx);
        ~LogicalExecutor() = default;

        // Logical operations
        void handleAnd();
        void handleOr();
        void handleNot();

    private:
        ExecutionContext& context;

        // Helper method
        bool isTruthy(const value::Value& val) const;
    };
}
