#pragma once
#include "BuiltinFunction.hpp"

namespace environment::registry::builtin
{
    /**
     * @brief Built-in sqrt function
     *
     * Calculates the square root of a numeric value.
     * Accepts int or float arguments, returns float.
     */
    class SqrtFunction : public BuiltinFunction
    {
    public:
        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
