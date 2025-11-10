#pragma once
#include "BuiltinFunction.hpp"

namespace environment::registry::builtin
{
    /**
     * @brief Built-in indexOf function
     *
     * Finds the first occurrence of a substring in a string.
     * Takes 2 arguments: (haystack, needle)
     * Returns the index of the first occurrence, or -1 if not found.
     */
    class IndexOfFunction : public BuiltinFunction
    {
    public:
        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
