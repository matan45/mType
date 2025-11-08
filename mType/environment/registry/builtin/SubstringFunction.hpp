#pragma once
#include "BuiltinFunction.hpp"

namespace environment::registry::builtin
{
    /**
     * @brief Built-in substring function
     *
     * Extracts a substring from a string.
     * Takes 3 arguments: (string, startIndex, length)
     * Returns the substring.
     */
    class SubstringFunction : public BuiltinFunction
    {
    public:
        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
