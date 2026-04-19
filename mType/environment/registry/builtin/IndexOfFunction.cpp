// MYT-126: walled off under flag-on — variant accessors not migrated.
#ifndef MTYPE_TAGGED_VALUE
#include "IndexOfFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace environment::registry::builtin
{
    Value IndexOfFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 2)
        {
            throw errors::ArgumentException("indexOf expects exactly 2 arguments (haystack, needle)");
        }

        // Extract haystack string
        std::string haystack;
        std::visit([&haystack](const auto& value)
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>)
            {
                haystack = value;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, value::InternedString>)
            {
                haystack = value.getString();
            }
            else
            {
                throw errors::RuntimeException("indexOf: first argument (haystack) must be a string");
            }
        }, args[0]);

        // Extract needle string
        std::string needle;
        std::visit([&needle](const auto& value)
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>)
            {
                needle = value;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, value::InternedString>)
            {
                needle = value.getString();
            }
            else
            {
                throw errors::RuntimeException("indexOf: second argument (needle) must be a string");
            }
        }, args[1]);

        // Find the first occurrence
        size_t pos = haystack.find(needle);

        if (pos == std::string::npos)
        {
            return -1;  // Not found
        }

        return static_cast<int>(pos);
    }

    std::string IndexOfFunction::getName() const
    {
        return "indexOf";
    }
}

#endif
