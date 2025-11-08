#include "ToLowerCaseFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/RuntimeException.hpp"
#include <algorithm>
#include <cctype>

namespace environment::registry::builtin
{
    Value ToLowerCaseFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("toLowerCase expects exactly 1 argument");
        }

        std::string str;
        std::visit([&str](const auto& value)
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>)
            {
                str = value;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, value::InternedString>)
            {
                str = value.getString();
            }
            else
            {
                throw errors::RuntimeException("toLowerCase can only be called on strings");
            }
        }, args[0]);

        // Convert to lowercase
        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) { return std::tolower(c); });

        return str;
    }

    std::string ToLowerCaseFunction::getName() const
    {
        return "toLowerCase";
    }
}
