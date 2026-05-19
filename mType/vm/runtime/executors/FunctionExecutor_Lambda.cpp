#include "FunctionExecutor.hpp"
#include "../../../environment/Environment.hpp"
#include "../../../value/ObjectInstance.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include "../../../value/ValueShim.hpp"

namespace vm::runtime
{
    void FunctionExecutor::convertLambdaArgumentsToInterfaces(
        std::span<value::Value> args,
        const std::vector<std::string>& parameterTypes
    )
    {
        // Bytecode VM uses BytecodeLambda which ObjectExecutor::handleCallMethod
        // invokes directly — no interface wrapping is performed here.

        // Instance methods carry `this` as the first parameter type, but args
        // does not, so skip the first slot when the type list is longer.
        size_t paramOffset = 0;
        if (parameterTypes.size() > args.size()) {
            paramOffset = 1;
        }

        for (size_t i = 0; i < args.size(); ++i)
        {
            size_t paramIndex = i + paramOffset;
            if (paramIndex >= parameterTypes.size()) break;

            const std::string& paramType = parameterTypes[paramIndex];
            value::Value& arg = args[i];

            // AUTO-UNBOXING: Int/Float/Bool/String wrappers → primitives.
            if (paramType == "int" && value::isObject(arg))
            {
                auto obj = value::asObject(arg);
                if (obj->getTypeName() == "Int")
                {
                    value::Value unboxedValue = obj->getFieldValue("value");
                    if (value::isInt(unboxedValue))
                    {
                        arg = unboxedValue;
                    }
                }
                continue;
            }
            else if (paramType == "float" && value::isObject(arg))
            {
                auto obj = value::asObject(arg);
                if (obj->getTypeName() == "Float")
                {
                    value::Value unboxedValue = obj->getFieldValue("value");
                    if (value::isFloat(unboxedValue))
                    {
                        arg = unboxedValue;
                    }
                }
                continue;
            }
            else if (paramType == "bool" && value::isObject(arg))
            {
                auto obj = value::asObject(arg);
                if (obj->getTypeName() == "Bool")
                {
                    value::Value unboxedValue = obj->getFieldValue("value");
                    if (value::isBool(unboxedValue))
                    {
                        arg = unboxedValue;
                    }
                }
                continue;
            }
            else if (paramType == "string" && value::isObject(arg))
            {
                auto obj = value::asObject(arg);
                if (obj->getTypeName() == "String")
                {
                    value::Value unboxedValue = obj->getFieldValue("value");
                    // MYT-317: accept any string form when unwrapping String box.
                    if (value::isAnyString(unboxedValue))
                    {
                        arg = unboxedValue;
                    }
                }
                continue;
            }

            // AUTO-BOXING: primitives → Int/Float/Bool/String wrappers.
            if (paramType == "Int" && value::isInt(arg))
            {
                auto intClass = environment->findClass("Int");
                if (intClass)
                {
                    auto instance = value::ObjectInstancePool::getInstance().acquire(intClass);
                    instance->setField("value", arg);
                    arg = instance;
                }
                continue;
            }
            else if (paramType == "Float" && value::isFloat(arg))
            {
                auto floatClass = environment->findClass("Float");
                if (floatClass)
                {
                    auto instance = value::ObjectInstancePool::getInstance().acquire(floatClass);
                    instance->setField("value", arg);
                    arg = instance;
                }
                continue;
            }
            else if (paramType == "Bool" && value::isBool(arg))
            {
                auto boolClass = environment->findClass("Bool");
                if (boolClass)
                {
                    auto instance = value::ObjectInstancePool::getInstance().acquire(boolClass);
                    instance->setField("value", arg);
                    arg = instance;
                }
                continue;
            }
            else if (paramType == "String" && value::isAnyString(arg))
            {
                auto stringClass = environment->findClass("String");
                if (stringClass)
                {
                    auto instance = value::ObjectInstancePool::getInstance().acquire(stringClass);
                    instance->setField("value", arg);
                    arg = instance;
                }
                continue;
            }

            // BytecodeLambda: no conversion needed — ObjectExecutor::handleCallMethod
            // invokes BytecodeLambda directly.
            if (value::isLambda(arg))
            {
                continue;
            }
        }
    }
}
