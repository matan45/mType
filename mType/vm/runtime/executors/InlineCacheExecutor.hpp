#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../jit/ic/InlineCacheTable.hpp"
#include "../../jit/ic/InlineCacheTypes.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/NullPointerException.hpp"
#include "../../../errors/FieldNotFoundException.hpp"

namespace vm::runtime
{
    class ObjectExecutor;
    class FunctionExecutor;

    class InlineCacheExecutor
    {
    public:
        explicit InlineCacheExecutor(ExecutionContext& ctx,
                                      vm::jit::ic::InlineCacheTable& icTable);

        void setObjectExecutor(ObjectExecutor* objExec) { objectExecutor = objExec; }
        void setFunctionExecutor(FunctionExecutor* funcExec) { functionExecutor = funcExec; }

        // IC-enabled field access
        void handleGetFieldIC(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSetFieldIC(const bytecode::BytecodeProgram::Instruction& instr);
        void handleInlineSetFieldIC(const bytecode::BytecodeProgram::Instruction& instr);
        void handleInlineGetFieldIC(const bytecode::BytecodeProgram::Instruction& instr);

        // IC-enabled method dispatch
        void handleCallMethodIC(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;
        vm::jit::ic::InlineCacheTable& icTable;
        ObjectExecutor* objectExecutor = nullptr;
        FunctionExecutor* functionExecutor = nullptr;
    };
}
