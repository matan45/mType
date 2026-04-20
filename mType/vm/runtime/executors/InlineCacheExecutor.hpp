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

        // MYT-173: CALL_METHOD_CACHED fast path. Reads the cached target embedded
        // on the Instruction, does a single shape compare against cachedShape, and
        // dispatches directly (no IC hashmap probe, no per-entry linear scan). On
        // shape miss, rewrites the opcode back to CALL_METHOD and re-enters the
        // generic IC path. Promotion is driven by tryPromoteToCached once the IC
        // transitions to MONOMORPHIC.
        void handleCallMethodCached(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        // Shared dispatch body used by the MONO-hit branch of handleCallMethodIC and
        // by handleCallMethodCached. Pops args + receiver, pushes a CallFrame, and
        // sets the instruction pointer to the callee's start offset (minus one, the
        // dispatch loop increments). Assumes funcMeta is non-native with a non-zero
        // startOffset (callers pre-filter).
        void dispatchDirectFromCachedTarget(
            const bytecode::BytecodeProgram::FunctionMetadata* funcMeta,
            size_t startOffset,
            const bytecode::BytecodeProgram* program,
            size_t programIndex,
            const std::string& qualifiedName,
            value::Value objectValue,
            size_t argCount);

        // MYT-173: promote CALL_METHOD -> CALL_METHOD_CACHED once the IC at the
        // current IP has stabilized to MONOMORPHIC with a bytecode callee. No-op if
        // the site has already demoted once (sticky via cachedDeoptCount).
        void tryPromoteToCached(
            const bytecode::BytecodeProgram::Instruction& instr,
            const vm::jit::ic::MethodICEntry& entry);

        // MYT-173: shape miss on a CACHED site. Rewrites the opcode back to
        // CALL_METHOD, clears cachedShape, bumps cachedDeoptCount (clamped), and
        // re-dispatches through handleCallMethodIC so the IC observes the new shape.
        void deoptAndReprocess(const bytecode::BytecodeProgram::Instruction& instr);

        ExecutionContext& context;
        vm::jit::ic::InlineCacheTable& icTable;
        ObjectExecutor* objectExecutor = nullptr;
        FunctionExecutor* functionExecutor = nullptr;
    };
}
