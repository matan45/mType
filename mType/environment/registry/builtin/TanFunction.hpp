#pragma once
#include "BuiltinFunction.hpp"

namespace environment::registry::builtin
{
    /**
     * @brief Built-in tan function
     *
     * Calculates the tangent of an angle in radians.
     * Accepts int or float arguments, returns float.
     */
    class TanFunction : public BuiltinFunction
    {
    public:
        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
