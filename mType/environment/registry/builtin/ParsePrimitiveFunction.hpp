#pragma once
#include "BuiltinFunction.hpp"

namespace environment::registry::builtin
{
    /**
     * @brief Built-in parsePrimitive function
     *
     * Converts a value to its string representation.
     * Handles all primitive types (int, float, bool, string) and converts them to strings.
     */
    class ParsePrimitiveFunction : public BuiltinFunction
    {
    public:
        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
