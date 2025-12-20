#pragma once
#include "BuiltinFunction.hpp"

namespace environment::registry::builtin
{
    /**
     * @brief Built-in cos function
     *
     * Calculates the cosine of an angle in radians.
     * Accepts int or float arguments, returns float.
     */
    class CosFunction : public BuiltinFunction
    {
    public:
        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
