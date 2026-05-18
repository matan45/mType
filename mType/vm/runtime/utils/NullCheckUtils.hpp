#pragma once
#include "../../../value/ValueType.hpp"
#include <cstddef>
#include "../../../value/ValueShim.hpp"
#include "../../../errors/NullPointerException.hpp"
#include "../../../value/ObjectInstance.hpp"
#include "../../../value/ValueObject.hpp"
#include "../context/ExecutionContext.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include "../../../environment/Environment.hpp"
#include "ErrorLocationHelper.hpp"

namespace vm::runtime::utils
{
    /**
     * Checks whether a Value represents null (either std::nullptr_t or std::monostate)
     */
    inline bool isNullValue(const value::Value& val)
    {
        return value::isNullType(val) ||
               value::isVoid(val);
    }

    /**
     * Guarded null receiver check: skips if INSTR_FLAG_NONNULL_RECEIVER is set.
     * Throws NullPointerException with source location when null is detected.
     */
    inline void checkNullReceiver(
        const bytecode::BytecodeProgram::Instruction& instr,
        const value::Value& objectValue,
        ExecutionContext& context,
        const std::shared_ptr<environment::Environment>& env,
        const std::string& operation,
        const std::string& name)
    {
        if (!(instr.flags & bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER))
        {
            if (isNullValue(objectValue))
            {
                ErrorLocationHelper::throwUserException(context, env,
                    "NullPointerException",
                    "Cannot " + operation + " '" + name + "' on null object");
            }
        }
    }

    /**
     * Evaluates truthiness of a Value.
     * Handles null, primitives, boxed Bool objects, and Bool value objects.
     */
    inline bool isTruthy(const value::Value& val)
    {
        if (value::isBool(val))
        {
            return value::asBool(val);
        }
        if (value::isInt(val))
        {
            return value::asInt(val) != 0;
        }
        if (isNullValue(val))
        {
            return false;
        }
        // Check if it's a Bool object (auto-boxed boolean)
        if (value::isObject(val))
        {
            auto obj = value::asObject(val);
            if (obj && obj->getClassDefinition()->getName() == "Bool")
            {
                value::Value valueField = obj->getFieldValue("value");
                if (value::isBool(valueField))
                {
                    return value::asBool(valueField);
                }
            }
        }
        // Check if it's a Bool value object (when Bool becomes a value class)
        if (value::isValueObject(val))
        {
            auto obj = value::asValueObject(val);
            if (obj && obj->getClassName() == "Bool")
            {
                value::Value valueField = obj->getFieldValue("value");
                if (value::isBool(valueField))
                {
                    return value::asBool(valueField);
                }
            }
        }
        return true;  // Objects, strings, etc. are truthy
    }
}
