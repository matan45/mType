#include "ComparisonExecutor.hpp"
#include <cstddef>
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../value/ValueShim.hpp"
#include "../utils/NullCheckUtils.hpp"
#include <iostream>

namespace vm::runtime
{
    ComparisonExecutor::ComparisonExecutor(ExecutionContext& ctx)
        : context(ctx)
    {}

    value::Value ComparisonExecutor::unboxIfNeeded(const value::Value& val) const {
        // Auto-unbox boxed types (Int, Float, Bool, String) to primitives
        if (value::isObject(val)) {
            auto obj = value::asObject(val);
            if (obj->getPrimitiveTag() != value::PrimitiveTypeTag::NONE) {
                return obj->getFieldValue("value");
            }
        }
        // Auto-unbox value object boxed types (when boxing classes become value classes)
        if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
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
        if (value::isValueObject(left) &&
            value::isValueObject(right)) {
            auto leftObj = value::asValueObject(left);
            auto rightObj = value::asValueObject(right);
            context.stackManager->push(leftObj->equals(*rightObj));
            return;
        }

        // Handle cross-type string comparison (std::string vs InternedString)
        if (value::isString(left) && value::isInternedString(right)) {
            context.stackManager->push(value::asInternedString(right) == value::asString(left));
            return;
        }
        if (value::isInternedString(left) && value::isString(right)) {
            context.stackManager->push(value::asInternedString(left) == value::asString(right));
            return;
        }

        // Handle bool-to-int conversion for comparisons
        if (value::isBool(left) && value::isInt(right)) {
            context.stackManager->push(static_cast<int>(value::asBool(left)) == value::asInt(right));
        } else if (value::isInt(left) && value::isBool(right)) {
            context.stackManager->push(value::asInt(left) == static_cast<int>(value::asBool(right)));
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
        if (value::isValueObject(left) &&
            value::isValueObject(right)) {
            auto leftObj = value::asValueObject(left);
            auto rightObj = value::asValueObject(right);
            context.stackManager->push(!leftObj->equals(*rightObj));
            return;
        }

        // Handle cross-type string comparison (std::string vs InternedString)
        if (value::isString(left) && value::isInternedString(right)) {
            context.stackManager->push(value::asInternedString(right) != value::asString(left));
            return;
        }
        if (value::isInternedString(left) && value::isString(right)) {
            context.stackManager->push(value::asInternedString(left) != value::asString(right));
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

        if (value::isInt(unboxedLeft) && value::isInt(unboxedRight)) {
            context.stackManager->push(value::asInt(unboxedLeft) < value::asInt(unboxedRight));
        } else if (value::isFloat(unboxedLeft) && value::isFloat(unboxedRight)) {
            context.stackManager->push(value::asFloat(unboxedLeft) < value::asFloat(unboxedRight));
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

        if (value::isInt(unboxedLeft) && value::isInt(unboxedRight)) {
            context.stackManager->push(value::asInt(unboxedLeft) > value::asInt(unboxedRight));
        } else if (value::isFloat(unboxedLeft) && value::isFloat(unboxedRight)) {
            context.stackManager->push(value::asFloat(unboxedLeft) > value::asFloat(unboxedRight));
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

        if (value::isInt(unboxedLeft) && value::isInt(unboxedRight)) {
            context.stackManager->push(value::asInt(unboxedLeft) <= value::asInt(unboxedRight));
        } else if (value::isFloat(unboxedLeft) && value::isFloat(unboxedRight)) {
            context.stackManager->push(value::asFloat(unboxedLeft) <= value::asFloat(unboxedRight));
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

        if (value::isInt(unboxedLeft) && value::isInt(unboxedRight)) {
            context.stackManager->push(value::asInt(unboxedLeft) >= value::asInt(unboxedRight));
        } else if (value::isFloat(unboxedLeft) && value::isFloat(unboxedRight)) {
            context.stackManager->push(value::asFloat(unboxedLeft) >= value::asFloat(unboxedRight));
        } else {
            throw errors::RuntimeException("GE requires numeric operands");
        }
    }
}
