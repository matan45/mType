#include "ComparisonExecutor.hpp"

namespace vm::runtime
{
    ComparisonExecutor::ComparisonExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    void ComparisonExecutor::handleEq() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        // Handle bool-to-int conversion for comparisons
        if (std::holds_alternative<bool>(left) && std::holds_alternative<int>(right)) {
            context.stackManager->push(static_cast<int>(std::get<bool>(left)) == std::get<int>(right));
        } else if (std::holds_alternative<int>(left) && std::holds_alternative<bool>(right)) {
            context.stackManager->push(std::get<int>(left) == static_cast<int>(std::get<bool>(right)));
        } else {
            // Direct equality comparison
            context.stackManager->push(left == right);
        }
    }

    void ComparisonExecutor::handleNe() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();
        context.stackManager->push(left != right);
    }

    void ComparisonExecutor::handleLt() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            context.stackManager->push(std::get<int>(left) < std::get<int>(right));
        } else if (std::holds_alternative<float>(left) && std::holds_alternative<float>(right)) {
            context.stackManager->push(std::get<float>(left) < std::get<float>(right));
        } else {
            throw errors::RuntimeException("LT requires numeric operands");
        }
    }

    void ComparisonExecutor::handleGt() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            context.stackManager->push(std::get<int>(left) > std::get<int>(right));
        } else if (std::holds_alternative<float>(left) && std::holds_alternative<float>(right)) {
            context.stackManager->push(std::get<float>(left) > std::get<float>(right));
        } else {
            throw errors::RuntimeException("GT requires numeric operands");
        }
    }

    void ComparisonExecutor::handleLe() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            context.stackManager->push(std::get<int>(left) <= std::get<int>(right));
        } else if (std::holds_alternative<float>(left) && std::holds_alternative<float>(right)) {
            context.stackManager->push(std::get<float>(left) <= std::get<float>(right));
        } else {
            throw errors::RuntimeException("LE requires numeric operands");
        }
    }

    void ComparisonExecutor::handleGe() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();
        if (std::holds_alternative<int>(left) && std::holds_alternative<int>(right)) {
            context.stackManager->push(std::get<int>(left) >= std::get<int>(right));
        } else if (std::holds_alternative<float>(left) && std::holds_alternative<float>(right)) {
            context.stackManager->push(std::get<float>(left) >= std::get<float>(right));
        } else {
            throw errors::RuntimeException("GE requires numeric operands");
        }
    }
}
