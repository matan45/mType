#include "Atan2Function.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/TypeException.hpp"
#include <cmath>

namespace environment::registry::builtin
{
    Value Atan2Function::execute(const std::vector<Value>& args)
    {
        if (args.size() != 2)
        {
            throw errors::ArgumentException("atan2 expects exactly 2 arguments (y, x)");
        }

        double y = 0.0;
        double x = 0.0;

        // Extract y value
        std::visit([&y](const auto& value)
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, double>)
            {
                y = value;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int64_t>)
            {
                y = static_cast<double>(value);
            }
            else
            {
                throw errors::TypeException("atan2 expects numeric arguments");
            }
        }, args[0]);

        // Extract x value
        std::visit([&x](const auto& value)
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, double>)
            {
                x = value;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int64_t>)
            {
                x = static_cast<double>(value);
            }
            else
            {
                throw errors::TypeException("atan2 expects numeric arguments");
            }
        }, args[1]);

        return std::atan2(y, x);
    }

    std::string Atan2Function::getName() const
    {
        return "atan2";
    }
}
