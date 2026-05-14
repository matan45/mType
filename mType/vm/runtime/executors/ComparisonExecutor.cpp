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

    // MYT-318: binary-compare handlers below collapse pop+pop+push into a
    // single vector mutation via binaryReplaceTop. Operands are read through
    // peekRef (no copy), the boolean result overwrites the new top in place.

    void ComparisonExecutor::handleEq() {
        const auto& right = context.stackManager->peekRef(0);
        const auto& left  = context.stackManager->peekRef(1);

        bool result;
        bool leftIsNull = utils::isNullValue(left);
        bool rightIsNull = utils::isNullValue(right);

        if (leftIsNull || rightIsNull) {
            result = (leftIsNull && rightIsNull);
        } else if (value::isValueObject(left) && value::isValueObject(right)) {
            auto leftObj  = value::asValueObject(left);
            auto rightObj = value::asValueObject(right);
            result = leftObj->equals(*rightObj);
        } else if (value::isString(left) && value::isInternedString(right)) {
            result = (value::asInternedString(right) == value::asString(left));
        } else if (value::isInternedString(left) && value::isString(right)) {
            result = (value::asInternedString(left) == value::asString(right));
        } else if (value::isBool(left) && value::isInt(right)) {
            result = (static_cast<int>(value::asBool(left)) == value::asInt(right));
        } else if (value::isInt(left) && value::isBool(right)) {
            result = (value::asInt(left) == static_cast<int>(value::asBool(right)));
        } else {
            result = (left == right);
        }
        context.stackManager->binaryReplaceTop(result);
    }

    void ComparisonExecutor::handleNe() {
        const auto& right = context.stackManager->peekRef(0);
        const auto& left  = context.stackManager->peekRef(1);

        bool result;
        bool leftIsNull = utils::isNullValue(left);
        bool rightIsNull = utils::isNullValue(right);

        if (leftIsNull || rightIsNull) {
            result = !(leftIsNull && rightIsNull);
        } else if (value::isValueObject(left) && value::isValueObject(right)) {
            auto leftObj  = value::asValueObject(left);
            auto rightObj = value::asValueObject(right);
            result = !leftObj->equals(*rightObj);
        } else if (value::isString(left) && value::isInternedString(right)) {
            result = (value::asInternedString(right) != value::asString(left));
        } else if (value::isInternedString(left) && value::isString(right)) {
            result = (value::asInternedString(left) != value::asString(right));
        } else {
            result = (left != right);
        }
        context.stackManager->binaryReplaceTop(result);
    }

    void ComparisonExecutor::handleLt() {
        // unboxIfNeeded returns by value so subsequent stack mutation is safe.
        value::Value unboxedLeft  = unboxIfNeeded(context.stackManager->peekRef(1));
        value::Value unboxedRight = unboxIfNeeded(context.stackManager->peekRef(0));

        bool result;
        if (value::isInt(unboxedLeft) && value::isInt(unboxedRight)) {
            result = (value::asInt(unboxedLeft) < value::asInt(unboxedRight));
        } else if (value::isFloat(unboxedLeft) && value::isFloat(unboxedRight)) {
            result = (value::asFloat(unboxedLeft) < value::asFloat(unboxedRight));
        } else {
            throw errors::RuntimeException("LT requires numeric operands");
        }
        context.stackManager->binaryReplaceTop(result);
    }

    void ComparisonExecutor::handleGt() {
        value::Value unboxedLeft  = unboxIfNeeded(context.stackManager->peekRef(1));
        value::Value unboxedRight = unboxIfNeeded(context.stackManager->peekRef(0));

        bool result;
        if (value::isInt(unboxedLeft) && value::isInt(unboxedRight)) {
            result = (value::asInt(unboxedLeft) > value::asInt(unboxedRight));
        } else if (value::isFloat(unboxedLeft) && value::isFloat(unboxedRight)) {
            result = (value::asFloat(unboxedLeft) > value::asFloat(unboxedRight));
        } else {
            throw errors::RuntimeException("GT requires numeric operands");
        }
        context.stackManager->binaryReplaceTop(result);
    }

    void ComparisonExecutor::handleLe() {
        value::Value unboxedLeft  = unboxIfNeeded(context.stackManager->peekRef(1));
        value::Value unboxedRight = unboxIfNeeded(context.stackManager->peekRef(0));

        bool result;
        if (value::isInt(unboxedLeft) && value::isInt(unboxedRight)) {
            result = (value::asInt(unboxedLeft) <= value::asInt(unboxedRight));
        } else if (value::isFloat(unboxedLeft) && value::isFloat(unboxedRight)) {
            result = (value::asFloat(unboxedLeft) <= value::asFloat(unboxedRight));
        } else {
            throw errors::RuntimeException("LE requires numeric operands");
        }
        context.stackManager->binaryReplaceTop(result);
    }

    void ComparisonExecutor::handleGe() {
        value::Value unboxedLeft  = unboxIfNeeded(context.stackManager->peekRef(1));
        value::Value unboxedRight = unboxIfNeeded(context.stackManager->peekRef(0));

        bool result;
        if (value::isInt(unboxedLeft) && value::isInt(unboxedRight)) {
            result = (value::asInt(unboxedLeft) >= value::asInt(unboxedRight));
        } else if (value::isFloat(unboxedLeft) && value::isFloat(unboxedRight)) {
            result = (value::asFloat(unboxedLeft) >= value::asFloat(unboxedRight));
        } else {
            throw errors::RuntimeException("GE requires numeric operands");
        }
        context.stackManager->binaryReplaceTop(result);
    }
}
