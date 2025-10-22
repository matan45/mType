#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../value/LambdaValue.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include <vector>
#include <string>

namespace vm::runtime
{
    /**
     * Helper class for method invocation operations
     * Extracted from ObjectExecutor to improve Single Responsibility Principle compliance
     * Handles: method calls, lambda invocation, super method calls, super constructor calls
     */
    class MethodInvocationHelper
    {
    public:
        explicit MethodInvocationHelper(ExecutionContext& ctx);
        ~MethodInvocationHelper() = default;

        // Method invocation operations
        void handleCallMethod(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperConstructor(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperInvoke(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;

        // Helper methods
        std::vector<value::Value> prepareMethodCallArguments(size_t argCount);
        void invokeLambdaMethod(std::shared_ptr<BytecodeLambda> lambda,
                               const std::string& methodName,
                               const std::vector<value::Value>& args);
        void invokeInstanceMethod(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                 const std::string& methodName,
                                 const std::vector<value::Value>& args);
    };
}
