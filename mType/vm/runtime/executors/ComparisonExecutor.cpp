#include "ComparisonExecutor.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../value/ValueObject.hpp"
#include "../utils/NullCheckUtils.hpp"
#include <iostream>

namespace vm::runtime
{
    ComparisonExecutor::ComparisonExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    value::Value ComparisonExecutor::unboxIfNeeded(const value::Value& val) const {
        // Auto-unbox boxed types (Int, Float, Bool, String) to primitives
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val)) {
            auto& obj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(val);
            if (obj->getPrimitiveTag() != value::PrimitiveTypeTag::NONE) {
                return obj->getFieldValue("value");
            }
        }
        // Auto-unbox value object boxed types (when boxing classes become value classes)
        if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(val)) {
            auto& obj = std::get<std::shared_ptr<value::ValueObject>>(val);
            if (obj->getPrimitiveTag() != value::PrimitiveTypeTag::NONE) {
                return obj->getFieldValue("value");
            }
        }
        return val;
    }

    void ComparisonExecutor::handleEq() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        // Handle null comparisons - both std::monostate and nullptr_t represent null
        bool leftIsNull = utils::isNullValue(left);
        bool rightIsNull = utils::isNullValue(right);

        if (leftIsNull || rightIsNull) {
            context.stackManager->push(leftIsNull && rightIsNull);
            return;
        }

        // Handle ValueObject structural equality
        if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(left) &&
            std::holds_alternative<std::shared_ptr<value::ValueObject>>(right)) {
            auto leftObj = std::get<std::shared_ptr<value::ValueObject>>(left);
            auto rightObj = std::get<std::shared_ptr<value::ValueObject>>(right);
            context.stackManager->push(leftObj->equals(*rightObj));
            return;
        }

        // Handle cross-type string comparison (std::string vs InternedString)
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<value::InternedString>(right)) {
            context.stackManager->push(std::get<value::InternedString>(right) == std::get<std::string>(left));
            return;
        }
        if (std::holds_alternative<value::InternedString>(left) && std::holds_alternative<std::string>(right)) {
            context.stackManager->push(std::get<value::InternedString>(left) == std::get<std::string>(right));
            return;
        }

        // Handle bool-to-int conversion for comparisons
        if (std::holds_alternative<bool>(left) && std::holds_alternative<int64_t>(right)) {
            context.stackManager->push(static_cast<int>(std::get<bool>(left)) == std::get<int64_t>(right));
        } else if (std::holds_alternative<int64_t>(left) && std::holds_alternative<bool>(right)) {
            context.stackManager->push(std::get<int64_t>(left) == static_cast<int>(std::get<bool>(right)));
        } else {
            // Direct equality comparison
            context.stackManager->push(left == right);
        }
    }

    void ComparisonExecutor::handleNe() {
        value::Value right = context.stackManager->pop();
        value::Value left = context.stackManager->pop();

        // Handle null comparisons - both std::monostate and nullptr_t represent null
        bool leftIsNull = utils::isNullValue(left);
        bool rightIsNull = utils::isNullValue(right);

        if (leftIsNull || rightIsNull) {
            // Not equal if one is null and the other isn't
            context.stackManager->push(!(leftIsNull && rightIsNull));
            return;
        }

        // Handle ValueObject structural inequality
        if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(left) &&
            std::holds_alternative<std::shared_ptr<value::ValueObject>>(right)) {
            auto leftObj = std::get<std::shared_ptr<value::ValueObject>>(left);
            auto rightObj = std::get<std::shared_ptr<value::ValueObject>>(right);
            context.stackManager->push(!leftObj->equals(*rightObj));
            return;
        }

        // Handle cross-type string comparison (std::string vs InternedString)
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<value::InternedString>(right)) {
            context.stackManager->push(std::get<value::InternedString>(right) != std::get<std::string>(left));
            return;
        }
        if (std::holds_alternative<value::InternedString>(left) && std::holds_alternative<std::string>(right)) {
            context.stackManager->push(std::get<value::InternedString>(left) != std::get<std::string>(right));
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

        if (std::holds_alternative<int64_t>(unboxedLeft) && std::holds_alternative<int64_t>(unboxedRight)) {
            context.stackManager->push(std::get<int64_t>(unboxedLeft) < std::get<int64_t>(unboxedRight));
        } else if (std::holds_alternative<double>(unboxedLeft) && std::holds_alternative<double>(unboxedRight)) {
            context.stackManager->push(std::get<double>(unboxedLeft) < std::get<double>(unboxedRight));
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

        if (std::holds_alternative<int64_t>(unboxedLeft) && std::holds_alternative<int64_t>(unboxedRight)) {
            context.stackManager->push(std::get<int64_t>(unboxedLeft) > std::get<int64_t>(unboxedRight));
        } else if (std::holds_alternative<double>(unboxedLeft) && std::holds_alternative<double>(unboxedRight)) {
            context.stackManager->push(std::get<double>(unboxedLeft) > std::get<double>(unboxedRight));
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

        if (std::holds_alternative<int64_t>(unboxedLeft) && std::holds_alternative<int64_t>(unboxedRight)) {
            context.stackManager->push(std::get<int64_t>(unboxedLeft) <= std::get<int64_t>(unboxedRight));
        } else if (std::holds_alternative<double>(unboxedLeft) && std::holds_alternative<double>(unboxedRight)) {
            context.stackManager->push(std::get<double>(unboxedLeft) <= std::get<double>(unboxedRight));
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

        if (std::holds_alternative<int64_t>(unboxedLeft) && std::holds_alternative<int64_t>(unboxedRight)) {
            context.stackManager->push(std::get<int64_t>(unboxedLeft) >= std::get<int64_t>(unboxedRight));
        } else if (std::holds_alternative<double>(unboxedLeft) && std::holds_alternative<double>(unboxedRight)) {
            context.stackManager->push(std::get<double>(unboxedLeft) >= std::get<double>(unboxedRight));
        } else {
            throw errors::RuntimeException("GE requires numeric operands");
        }
    }
}
