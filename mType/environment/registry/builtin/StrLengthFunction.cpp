#include "StrLengthFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace environment::registry::builtin
{
    Value StrLengthFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("str::length expects exactly 1 argument");
        }

        return std::visit([](const auto& value) -> Value
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>)
            {
                return static_cast<int>(value.length());
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, value::InternedString>)
            {
                return static_cast<int>(value.getString().length());
            }
            else
            {
                throw errors::RuntimeException("str::length can only be called on strings");
            }
        }, args[0]);
    }

    std::string StrLengthFunction::getName() const
    {
        return "strLength";
    }
}
