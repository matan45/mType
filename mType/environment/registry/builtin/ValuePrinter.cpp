#include "ValuePrinter.hpp"
#include "../../../value/AsyncPromiseValue.hpp"
#include "../../../value/ValueObject.hpp"

namespace environment::registry::builtin
{
    ValuePrinter::ValuePrinter(MethodCallHandler handler)
        : methodCallHandler(handler)
    {
    }

    void ValuePrinter::print(const Value& value, std::ostream& out) const
    {
        std::visit([this, &out](const auto& v)
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>)
            {
                out << v;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, value::InternedString>)
            {
                out << v.getString();
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int64_t>)
            {
                out << v;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, double>)
            {
                out << v;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>)
            {
                out << (v ? "true" : "false");
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::monostate>)
            {
                out << "null";
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, nullptr_t>)
            {
                out << "null";
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<value::PromiseValue>>)
            {
                if (!v)
                {
                    out << "null";
                }
                else if (v->isFulfilled())
                {
                    out << "[Promise:fulfilled]";
                }
                else if (v->isPending())
                {
                    out << "[Promise:pending]";
                }
                else
                {
                    out << "[Promise:rejected]";
                }
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>)
            {
                if (!v)
                {
                    out << "null";
                }
                else
                {
                    // Check if this is a primitive wrapper (String, Int, Bool, Float) with a 'value' field
                    auto primitiveValue = tryGetPrimitiveWrapperValue(v);
                    if (primitiveValue.has_value())
                    {
                        // Recursively print the unwrapped primitive value
                        print(primitiveValue.value(), out);
                    }
                    else
                    {
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
                    }
                }
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<value::ValueObject>>)
            {
                if (!v)
                {
                    out << "null";
                }
                else
                {
                    // Check if this is a primitive wrapper value class (String, Int, Bool, Float)
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
                }
            }
        }, value);
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

            if (std::holds_alternative<std::string>(result))
            {
                return std::get<std::string>(result);
            }

            if (std::holds_alternative<value::InternedString>(result))
            {
                return std::get<value::InternedString>(result).getString();
            }

            if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(result))
            {
                auto resultObj = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(result);
                if (resultObj)
                {
                    Value fieldValue = resultObj->getFieldValue("value");

                    if (std::holds_alternative<std::string>(fieldValue))
                    {
                        return std::get<std::string>(fieldValue);
                    }

                    if (std::holds_alternative<value::InternedString>(fieldValue))
                    {
                        return std::get<value::InternedString>(fieldValue).getString();
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
