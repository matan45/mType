#pragma once
#include "BuiltinFunction.hpp"

namespace environment::registry::builtin
{
    /**
     * @brief Built-in strLength function
     *
     * Returns the length of a string.
     * Only accepts string or InternedString arguments.
     */
    class StrLengthFunction : public BuiltinFunction
    {
    public:
        Value execute(const std::vector<Value>& args) override;
        std::string getName() const override;
    };
}
