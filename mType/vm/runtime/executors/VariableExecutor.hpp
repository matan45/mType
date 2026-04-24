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

        // Slot-based fast-path entry used by MYT-202 fused opcode handlers.
        // Skips the Instruction::operands shim construction — the fused
        // dispatch already has the raw slot index from its own operands.
        // Semantics identical to handleLoadLocal / handleStoreLocal's
        // no-varName path; tryPromote{Load,Store}Local still correctly bails
        // when the current IP holds a fused opcode, not the generic form.
        void loadLocalSlot(size_t slot);
        void storeLocalSlot(size_t slot);

        // MYT-199: type-quickened LOAD_LOCAL / STORE_LOCAL. The generic
        // handlers above record their first observed ValueType and rewrite
        // the opcode in place to one of these variants. The specialized
        // handler guards on the expected tag; on a guard miss it demotes
        // back to the generic opcode with a sticky counter so the site
        // isn't re-promoted (mirrors MYT-173's CALL_METHOD_CACHED policy).
        void handleLoadLocalInt(const bytecode::BytecodeProgram::Instruction& instr);
        void handleLoadLocalFloat(const bytecode::BytecodeProgram::Instruction& instr);
        void handleLoadLocalBool(const bytecode::BytecodeProgram::Instruction& instr);
        void handleLoadLocalBoxedInst(const bytecode::BytecodeProgram::Instruction& instr);
        void handleStoreLocalInt(const bytecode::BytecodeProgram::Instruction& instr);
        void handleStoreLocalFloat(const bytecode::BytecodeProgram::Instruction& instr);
        void handleStoreLocalBool(const bytecode::BytecodeProgram::Instruction& instr);
        void handleStoreLocalBoxedInst(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;

        // Helper methods for handleLoadVar
        bool tryLoadFromInstanceField(const std::string& varName);
        bool tryLoadFromStaticField(const std::string& varName);

        // Helper methods for handleStoreVar
        bool tryStoreToInstanceField(const std::string& varName, const value::Value& val);
        bool tryStoreToStaticField(const std::string& varName, const value::Value& val);
        void validateAndStoreGlobalVariable(const std::string& varName,
                                           const value::Value& val,
                                           std::shared_ptr<runtimeTypes::global::VariableDefinition> varDef);

        // MYT-199 helpers. See .cpp for policy details.
        bool isCurrentFrameSimple() const;
        void tryPromoteLoadLocal(value::ValueType observedTag);
        void tryPromoteStoreLocal(value::ValueType observedTag);
        void deoptLoadLocal(const bytecode::BytecodeProgram::Instruction& instr);
        void deoptStoreLocal(const bytecode::BytecodeProgram::Instruction& instr);
        void handleLoadLocalSpecialized(const bytecode::BytecodeProgram::Instruction& instr,
                                        value::ValueType expectedTag);
        void handleStoreLocalSpecialized(const bytecode::BytecodeProgram::Instruction& instr,
                                         value::ValueType expectedTag);
    };
}
