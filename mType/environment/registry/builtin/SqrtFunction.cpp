// MYT-126: walled off under flag-on — variant accessors not migrated.
#ifndef MTYPE_TAGGED_VALUE
#include "SqrtFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/TypeException.hpp"
#include <cmath>

namespace environment::registry::builtin
{
    Value SqrtFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("sqrt expects exactly 1 argument");
        }

        return std::visit([](const auto& value) -> Value
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, double>)
            {
                return std::sqrt(value);
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int64_t>)
            {
                return std::sqrt(static_cast<double>(value));
            }
            else
            {
                throw errors::TypeException("sqrt expects a numeric argument");
            }
        }, args[0]);
    }

    std::string SqrtFunction::getName() const
    {
        return "sqrt";
    }
}

#endif
