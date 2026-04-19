// MYT-126: walled off under flag-on — variant accessors not migrated.
#ifndef MTYPE_TAGGED_VALUE
#include "SubstringFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace environment::registry::builtin
{
    Value SubstringFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 3)
        {
            throw errors::ArgumentException("substring expects exactly 3 arguments (string, startIndex, length)");
        }

        // Extract the string
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
                throw errors::RuntimeException("substring: first argument must be a string");
            }
        }, args[0]);

        // Extract startIndex
        int64_t startIndex = 0;
        std::visit([&startIndex](const auto& value)
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int64_t>)
            {
                startIndex = value;
            }
            else
            {
                throw errors::RuntimeException("substring: second argument (startIndex) must be an integer");
            }
        }, args[1]);

        // Extract length
        int64_t length = 0;
        std::visit([&length](const auto& value)
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int64_t>)
            {
                length = value;
            }
            else
            {
                throw errors::RuntimeException("substring: third argument (length) must be an integer");
            }
        }, args[2]);

        // Validate indices
        if (startIndex < 0)
        {
            throw errors::RuntimeException("substring: startIndex cannot be negative");
        }

        if (length < 0)
        {
            throw errors::RuntimeException("substring: length cannot be negative");
        }

        size_t strLen = str.length();
        if (static_cast<size_t>(startIndex) > strLen)
        {
            throw errors::RuntimeException("substring: startIndex out of bounds");
        }

        // Adjust length to not exceed string bounds
        size_t actualLength = static_cast<size_t>(length);
        if (static_cast<size_t>(startIndex) + actualLength > strLen)
        {
            actualLength = strLen - static_cast<size_t>(startIndex);
        }

        // Extract substring
        return str.substr(static_cast<size_t>(startIndex), actualLength);
    }

    std::string SubstringFunction::getName() const
    {
        return "substring";
    }
}

#endif
