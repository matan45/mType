#pragma once
#include "BuiltinFunction.hpp"

namespace environment::registry::builtin
{
    /**
     * @brief Built-in atan2 function
     *
     * Calculates the arc tangent of y/x using the signs of both arguments
     * to determine the quadrant of the result.
     * Accepts two numeric arguments (y, x), returns float in range [-PI, PI].
     */
    class Atan2Function : public BuiltinFunction
    {
    public:
        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
