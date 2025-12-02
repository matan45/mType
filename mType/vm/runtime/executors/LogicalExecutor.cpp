#include "LogicalExecutor.hpp"

namespace vm::runtime
{
    LogicalExecutor::LogicalExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    void LogicalExecutor::handleAnd() {
        bool right = isTruthy(context.stackManager->pop());
        bool left = isTruthy(context.stackManager->pop());
        context.stackManager->push(left && right);
    }

    void LogicalExecutor::handleOr() {
        bool right = isTruthy(context.stackManager->pop());
        bool left = isTruthy(context.stackManager->pop());
        context.stackManager->push(left || right);
    }

    void LogicalExecutor::handleNot() {
        context.stackManager->push(!isTruthy(context.stackManager->pop()));
    }

    bool LogicalExecutor::isTruthy(const value::Value& val) const {
        if (std::holds_alternative<bool>(val)) {
            return std::get<bool>(val);
        }
        if (std::holds_alternative<int64_t>(val)) {
            return std::get<int64_t>(val) != 0;
        }
        if (std::holds_alternative<nullptr_t>(val)) {
            return false;
        }
        if (std::holds_alternative<std::monostate>(val)) {
            return false;
        }
        return true;  // Objects, strings, etc. are truthy
    }
}
