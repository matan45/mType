#pragma once
#include "BuiltinFunction.hpp"

namespace environment::registry::builtin
{
    /**
     * @brief Built-in acos function
     *
     * Calculates the arc cosine (inverse cosine) in radians.
     * Accepts int or float arguments in range [-1, 1], returns float.
     */
    class AcosFunction : public BuiltinFunction
    {
    public:
        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
