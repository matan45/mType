#pragma once
#include "BuiltinFunction.hpp"

namespace environment::registry::builtin
{
    /**
     * @brief Built-in hashCode function
     *
     * Generates a hash code for any value type.
     * Returns a positive integer hash code based on the value's content.
     */
    class HashCodeFunction : public BuiltinFunction
    {
    public:
        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
