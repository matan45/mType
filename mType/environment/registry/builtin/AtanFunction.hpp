#pragma once
#include "BuiltinFunction.hpp"

namespace environment::registry::builtin
{
    /**
     * @brief Built-in atan function
     *
     * Calculates the arc tangent (inverse tangent) in radians.
     * Accepts int or float arguments, returns float in range [-PI/2, PI/2].
     */
    class AtanFunction : public BuiltinFunction
    {
    public:
        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
