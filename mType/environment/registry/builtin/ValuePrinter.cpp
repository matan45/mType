#include "ValuePrinter.hpp"
#include "../../../value/AsyncPromiseValue.hpp"

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
            else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int>)
            {
                out << v;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, float>)
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

        if (!toStringMethod || toStringMethod->isStatic())
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
        catch (...)
        {
            // If toString() fails, return nullopt to fall back to default representation
        }

        return std::nullopt;
    }
}
