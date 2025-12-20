#pragma once
#include "BuiltinFunction.hpp"

namespace environment::registry::builtin
{
    /**
     * @brief Built-in asin function
     *
     * Calculates the arc sine (inverse sine) in radians.
     * Accepts int or float arguments in range [-1, 1], returns float.
     */
    class AsinFunction : public BuiltinFunction
    {
    public:
        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
