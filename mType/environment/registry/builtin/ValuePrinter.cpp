#include "ValuePrinter.hpp"
#include "../../../value/AsyncPromiseValue.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../value/ValueShim.hpp"

namespace environment::registry::builtin
{
    ValuePrinter::ValuePrinter(MethodCallHandler handler)
        : methodCallHandler(handler)
    {
    }

    void ValuePrinter::print(const Value& value, std::ostream& out) const
    {
        if (value::isString(value))
        {
            out << value::asString(value);
            return;
        }
        if (value::isInternedString(value))
        {
            out << value::asInternedString(value).getString();
            return;
        }
        if (value::isInt(value))
        {
            out << value::asInt(value);
            return;
        }
        if (value::isFloat(value))
        {
            out << value::asFloat(value);
            return;
        }
        if (value::isBool(value))
        {
            out << (value::asBool(value) ? "true" : "false");
            return;
        }
        if (value::isVoid(value) || value::isNullType(value))
        {
            out << "null";
            return;
        }
        if (value::isPromise(value))
        {
            auto v = value::asPromise(value);
            if (!v) { out << "null"; return; }
            if (v->isFulfilled()) { out << "[Promise:fulfilled]"; return; }
            if (v->isPending()) { out << "[Promise:pending]"; return; }
            out << "[Promise:rejected]";
            return;
        }
        if (value::isObject(value))
        {
            auto v = value::asObject(value);
            if (!v) { out << "null"; return; }
            // Check if this is a primitive wrapper (String, Int, Bool, Float) with a 'value' field
            auto primitiveValue = tryGetPrimitiveWrapperValue(v);
            if (primitiveValue.has_value())
            {
                // Recursively print the unwrapped primitive value
                print(primitiveValue.value(), out);
                return;
            }
            // Try calling toString() for non-primitive objects
            auto stringRep = getObjectStringRepresentation(v);
            if (stringRep.has_value())
            {
                out << stringRep.value();
            }
            else
            {
                out << "[object " << v->getTypeName() << "]";
            }
            return;
        }
        if (value::isValueObject(value))
        {
            auto v = value::asValueObject(value);
            if (!v) { out << "null"; return; }
            const std::string& typeName = v->getClassName();
            if ((typeName == "String" || typeName == "Int" || typeName == "Bool" || typeName == "Float")
                && v->hasField("value"))
            {
                print(v->getFieldValue("value"), out);
            }
            else
            {
                out << "<" << typeName << ">";
            }
            return;
        }
        // Remaining kinds (NativeArray / multi-arrays / Lambda) — fall through
        // silently to match the flag-off std::visit which has no branch for
        // them (outputs nothing).
    }

    std::optional<std::string> ValuePrinter::getObjectStringRepresentation(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& value) const
    {
        if (!value || !methodCallHandler)
        {
            return std::nullopt;
        }

        auto classDef = value->getClassDefinition();
        auto toStringMethod = classDef ? classDef->findMethod("toString", 0) : nullptr;

        if (!toStringMethod)
        {
            return std::nullopt;
        }

        try
        {
            Value result = methodCallHandler(value, "toString", {});

            if (value::isString(result))
            {
                return value::asString(result);
            }

            if (value::isInternedString(result))
            {
                return value::asInternedString(result).getString();
            }

            if (value::isObject(result))
            {
                auto resultObj = value::asObject(result);
                if (resultObj)
                {
                    Value fieldValue = resultObj->getFieldValue("value");

                    if (value::isString(fieldValue))
                    {
                        return value::asString(fieldValue);
                    }

                    if (value::isInternedString(fieldValue))
                    {
                        return value::asInternedString(fieldValue).getString();
                    }
                }
            }
        }
        catch (const std::exception&)
        {
            // If toString() fails, return nullopt to fall back to default representation
        }
        catch (...)
        {
            // If toString() fails, return nullopt to fall back to default representation
        }

        return std::nullopt;
    }

    std::optional<Value> ValuePrinter::tryGetPrimitiveWrapperValue(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& value) const
    {
        if (!value)
        {
            return std::nullopt;
        }

        // Check if this is a primitive wrapper class (String, Int, Bool, Float)
        std::string typeName = value->getTypeName();
        if (typeName == "String" || typeName == "Int" || typeName == "Bool" || typeName == "Float")
        {
            try
            {
                // Try to get the 'value' field
                Value fieldValue = value->getFieldValue("value");
                return fieldValue;
            }
            catch (...)
            {
                // If getting field fails, it's not a valid wrapper
                return std::nullopt;
            }
        }

        return std::nullopt;
    }
}

