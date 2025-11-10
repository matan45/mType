#pragma once
#include "BuiltinFunction.hpp"

namespace environment::registry::builtin
{
    /**
     * @brief Built-in toLowerCase function
     *
     * Converts a string to lowercase.
     * Takes 1 argument: (string)
     * Returns the lowercase string.
     */
    class ToLowerCaseFunction : public BuiltinFunction
    {
    public:
        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
