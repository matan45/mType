#pragma once
#include "BuiltinFunction.hpp"

namespace environment::registry::builtin
{
    /**
     * @brief Built-in toUpperCase function
     *
     * Converts a string to uppercase.
     * Takes 1 argument: (string)
     * Returns the uppercase string.
     */
    class ToUpperCaseFunction : public BuiltinFunction
    {
    public:
        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
