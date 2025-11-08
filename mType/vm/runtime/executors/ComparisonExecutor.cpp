#include "ComparisonExecutor.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include <iostream>

namespace vm::runtime
{
    ComparisonExecutor::ComparisonExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    value::Value ComparisonExecutor::unboxIfNeeded(const value::Value& val) const {
        // Auto-unbox boxed types (Int, Float, Bool, String) to primitives
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
            auto obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
            std::string typeName = obj->getTypeName();
            if (typeName == "Int" || typeName == "Float" ||
                typeName == "Bool" || typeName == "String") {
                return obj->getFieldValue("value");
            }
        }
        return val;
    }

    void ComparisonExecutor::handleEq() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        // Handle null comparisons - both std::monostate and nullptr_t represent null
        bool leftIsNull = std::holds_alternative<std::monostate>(left) || std::holds_alternative<nullptr_t>(left);
        bool rightIsNull = std::holds_alternative<std::monostate>(right) || std::holds_alternative<nullptr_t>(right);

        if (leftIsNull || rightIsNull) {
            context.stackManager->push(leftIsNull && rightIsNull);
            return;
        }

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

        // Handle null comparisons - both std::monostate and nullptr_t represent null
        bool leftIsNull = std::holds_alternative<std::monostate>(left) || std::holds_alternative<nullptr_t>(left);
        bool rightIsNull = std::holds_alternative<std::monostate>(right) || std::holds_alternative<nullptr_t>(right);

        if (leftIsNull || rightIsNull) {
            // Not equal if one is null and the other isn't
            context.stackManager->push(!(leftIsNull && rightIsNull));
            return;
        }

        context.stackManager->push(left != right);
    }

    void ComparisonExecutor::handleLt() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        // Auto-unbox if needed
        value::Value unboxedLeft = unboxIfNeeded(left);
        value::Value unboxedRight = unboxIfNeeded(right);

        if (std::holds_alternative<int>(unboxedLeft) && std::holds_alternative<int>(unboxedRight)) {
            context.stackManager->push(std::get<int>(unboxedLeft) < std::get<int>(unboxedRight));
        } else if (std::holds_alternative<float>(unboxedLeft) && std::holds_alternative<float>(unboxedRight)) {
            context.stackManager->push(std::get<float>(unboxedLeft) < std::get<float>(unboxedRight));
        } else {
            throw errors::RuntimeException("LT requires numeric operands");
        }
    }

    void ComparisonExecutor::handleGt() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        // Auto-unbox if needed
        value::Value unboxedLeft = unboxIfNeeded(left);
        value::Value unboxedRight = unboxIfNeeded(right);

        if (std::holds_alternative<int>(unboxedLeft) && std::holds_alternative<int>(unboxedRight)) {
            context.stackManager->push(std::get<int>(unboxedLeft) > std::get<int>(unboxedRight));
        } else if (std::holds_alternative<float>(unboxedLeft) && std::holds_alternative<float>(unboxedRight)) {
            context.stackManager->push(std::get<float>(unboxedLeft) > std::get<float>(unboxedRight));
        } else {
            throw errors::RuntimeException("GT requires numeric operands");
        }
    }

    void ComparisonExecutor::handleLe() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        // Auto-unbox if needed
        value::Value unboxedLeft = unboxIfNeeded(left);
        value::Value unboxedRight = unboxIfNeeded(right);

        if (std::holds_alternative<int>(unboxedLeft) && std::holds_alternative<int>(unboxedRight)) {
            context.stackManager->push(std::get<int>(unboxedLeft) <= std::get<int>(unboxedRight));
        } else if (std::holds_alternative<float>(unboxedLeft) && std::holds_alternative<float>(unboxedRight)) {
            context.stackManager->push(std::get<float>(unboxedLeft) <= std::get<float>(unboxedRight));
        } else {
            throw errors::RuntimeException("LE requires numeric operands");
        }
    }

    void ComparisonExecutor::handleGe() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        // Auto-unbox if needed
        value::Value unboxedLeft = unboxIfNeeded(left);
        value::Value unboxedRight = unboxIfNeeded(right);

        if (std::holds_alternative<int>(unboxedLeft) && std::holds_alternative<int>(unboxedRight)) {
            context.stackManager->push(std::get<int>(unboxedLeft) >= std::get<int>(unboxedRight));
        } else if (std::holds_alternative<float>(unboxedLeft) && std::holds_alternative<float>(unboxedRight)) {
            context.stackManager->push(std::get<float>(unboxedLeft) >= std::get<float>(unboxedRight));
        } else {
            throw errors::RuntimeException("GE requires numeric operands");
        }
    }
}
