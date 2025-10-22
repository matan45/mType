#include "ParsePrimitiveFunction.hpp"
#include "../../../errors/ArgumentException.hpp"

namespace environment::registry::builtin
{
    Value ParsePrimitiveFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("parsePrimitive expects exactly 1 argument");
        }

        return std::visit([](const auto& value) -> Value
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>)
            {
                return value;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, value::InternedString>)
            {
                return value.getString();
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int>)
            {
                return std::to_string(value);
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, float>)
            {
                std::string str = std::to_string(value);
                str.erase(str.find_last_not_of('0') + 1, std::string::npos);
                if (str.back() == '.')
                {
                    str += '0';
                }
                return str;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, bool>)
            {
                return value ? std::string("true") : std::string("false");
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::monostate>)
            {
                return std::string("void");
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, nullptr_t>)
            {
                return std::string("null");
            }
            else
            {
                return std::string("unknown");
            }
        }, args[0]);
    }

    std::string ParsePrimitiveFunction::getName() const
    {
        return "parsePrimitive";
    }
}
