#include "AsinFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/TypeException.hpp"
#include <cmath>

namespace environment::registry::builtin
{
    Value AsinFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("asin expects exactly 1 argument");
        }

        return std::visit([](const auto& value) -> Value
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, double>)
            {
                return std::asin(value);
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int64_t>)
            {
                return std::asin(static_cast<double>(value));
            }
            else
            {
                throw errors::TypeException("asin expects a numeric argument");
            }
        }, args[0]);
    }

    std::string AsinFunction::getName() const
    {
        return "asin";
    }
}
