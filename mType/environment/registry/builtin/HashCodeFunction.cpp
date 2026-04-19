// MYT-126: walled off under flag-on — variant accessors not migrated.
#ifndef MTYPE_TAGGED_VALUE
#include "HashCodeFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"

namespace environment::registry::builtin
{
    Value HashCodeFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("hashCode expects exactly 1 argument");
        }

        return std::visit([](const auto& value) -> Value
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>)
            {
                std::hash<std::string> hasher;
                return static_cast<int64_t>(hasher(value) & 0x7FFFFFFF);
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, value::InternedString>)
            {
                std::hash<std::string> hasher;
                return static_cast<int64_t>(hasher(value.getString()) & 0x7FFFFFFF);
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int64_t>)
            {
                // Return 31-bit positive hash to prevent overflow when used in hash calculations
                std::hash<int64_t> hasher;
                return static_cast<int64_t>(hasher(value) & 0x7FFFFFFF);
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, double>)
            {
                std::hash<double> hasher;
                return static_cast<int64_t>(hasher(value) & 0x7FFFFFFF);
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, bool>)
            {
                return value ? 1231 : 1237;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>)
            {
                if (!value)
                {
                    return 0;
                }
                std::string contentHash = value->getContentHash();
                std::hash<std::string> hasher;
                return static_cast<int64_t>(hasher(contentHash) & 0x7FFFFFFF);
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, nullptr_t>)
            {
                return 0;
            }
            else
            {
                return 0;
            }
        }, args[0]);
    }

    std::string HashCodeFunction::getName() const
    {
        return "hashCode";
    }
}

#endif
