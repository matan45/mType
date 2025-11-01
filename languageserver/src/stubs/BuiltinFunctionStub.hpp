// Minimal stub for BuiltinFunction - just enough to complete the type
#pragma once
#include <string>
#include <vector>
#include "../../../mType/value/ValueType.hpp"

namespace environment::registry::builtin {

class BuiltinFunction {
public:
    virtual ~BuiltinFunction() = default;
    virtual value::Value execute(const std::vector<value::Value>& args) = 0;
    virtual std::string getName() const = 0;
};

} // namespace environment::registry::builtin
