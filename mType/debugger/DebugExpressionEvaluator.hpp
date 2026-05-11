#pragma once

#include "../value/ValueType.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace vm::runtime
{
    class VirtualMachine;
}

namespace debugger
{
    class VMVariableInspector;

    class DebugExpressionEvaluator
    {
    public:
        struct Result
        {
            bool success = false;
            value::Value value = std::monostate{};
            std::string error;
        };

        using Resolver = std::function<std::optional<value::Value>(const std::string&)>;

        static Result evaluate(std::shared_ptr<vm::runtime::VirtualMachine> vm,
                               VMVariableInspector& inspector,
                               const std::string& expression);

        static Result evaluateWithResolver(const std::string& expression,
                                           const Resolver& resolver);

        static bool isTruthy(const value::Value& val);
    };
}
