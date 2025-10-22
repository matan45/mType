#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../value/ValueType.hpp"
#include "../../../runtimeTypes/global/VariableDefinition.hpp"

namespace vm::runtime
{
    /**
     * Executes variable-related opcodes
     * Handles LOAD_VAR, STORE_VAR, DECLARE_VAR, LOAD_LOCAL, STORE_LOCAL
     * Manages global variables, local variables, and lambda closure mechanics
     */
    class VariableExecutor
    {
    public:
        explicit VariableExecutor(ExecutionContext& ctx);
        ~VariableExecutor() = default;

        // Global variable operations
        void handleLoadVar(const bytecode::BytecodeProgram::Instruction& instr);
        void handleStoreVar(const bytecode::BytecodeProgram::Instruction& instr);
        void handleDeclareVar(const bytecode::BytecodeProgram::Instruction& instr);

        // Local variable operations (stack-based)
        void handleLoadLocal(const bytecode::BytecodeProgram::Instruction& instr);
        void handleStoreLocal(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;

        // Helper methods for handleLoadVar
        bool tryLoadFromLambdaParentFrame(const std::string& varName);
        bool tryLoadFromInstanceField(const std::string& varName);
        bool tryLoadFromStaticField(const std::string& varName);

        // Helper methods for handleStoreVar
        bool tryStoreToInstanceField(const std::string& varName, const value::Value& val);
        bool tryStoreToStaticField(const std::string& varName, const value::Value& val);
        void validateAndStoreGlobalVariable(const std::string& varName,
                                           const value::Value& val,
                                           std::shared_ptr<runtimeTypes::global::VariableDefinition> varDef);
    };
}
