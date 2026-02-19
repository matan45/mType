#include "TanFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/TypeException.hpp"
#include <cmath>

namespace environment::registry::builtin
{
    Value TanFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("tan expects exactly 1 argument");
        }

        return std::visit([](const auto& value) -> Value
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, double>)
            {
                return std::tan(value);
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int64_t>)
            {
                return std::tan(static_cast<double>(value));
            }
            else
            {
                throw errors::TypeException("tan expects a numeric argument");
            }
        }, args[0]);
    }

    std::string TanFunction::getName() const
    {
        return "tan";
    }
}
