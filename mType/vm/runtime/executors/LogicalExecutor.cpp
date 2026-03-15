#include "LogicalExecutor.hpp"
#include "../utils/NullCheckUtils.hpp"

namespace vm::runtime
{
    LogicalExecutor::LogicalExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    void LogicalExecutor::handleAnd() {
        bool right = utils::isTruthy(context.stackManager->pop());
        bool left = utils::isTruthy(context.stackManager->pop());
        context.stackManager->push(left && right);
    }

    void LogicalExecutor::handleOr() {
        bool right = utils::isTruthy(context.stackManager->pop());
        bool left = utils::isTruthy(context.stackManager->pop());
        context.stackManager->push(left || right);
    }

    void LogicalExecutor::handleNot() {
        context.stackManager->push(!utils::isTruthy(context.stackManager->pop()));
    }
}
