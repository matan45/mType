#include "AtanFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/TypeException.hpp"
#include <cmath>

namespace environment::registry::builtin
{
    Value AtanFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("atan expects exactly 1 argument");
        }

        return std::visit([](const auto& value) -> Value
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, float>)
            {
                return std::atan(value);
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int64_t>)
            {
                return std::atan(static_cast<float>(value));
            }
            else
            {
                throw errors::TypeException("atan expects a numeric argument");
            }
        }, args[0]);
    }

    std::string AtanFunction::getName() const
    {
        return "atan";
    }
}
