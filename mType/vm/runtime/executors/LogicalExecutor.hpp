#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../utils/NullCheckUtils.hpp"

namespace vm::runtime
{
    /**
     * Executes logical opcodes
     * Handles AND, OR, NOT operations
     *
     * MYT-319: handler bodies inlined in the header so MSVC v145 (no /GL)
     * can inline them through the dispatch switch's unique_ptr deref.
     */
    class LogicalExecutor
    {
    public:
        explicit LogicalExecutor(ExecutionContext& ctx) : context(ctx) {}
        ~LogicalExecutor() = default;

        inline void handleAnd() {
            bool right = utils::isTruthy(context.stackManager->pop());
            bool left = utils::isTruthy(context.stackManager->pop());
            context.stackManager->push(left && right);
        }

        inline void handleOr() {
            bool right = utils::isTruthy(context.stackManager->pop());
            bool left = utils::isTruthy(context.stackManager->pop());
            context.stackManager->push(left || right);
        }

        inline void handleNot() {
            context.stackManager->push(!utils::isTruthy(context.stackManager->pop()));
        }

    private:
        ExecutionContext& context;
    };
}
