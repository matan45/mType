#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/UserException.hpp"

namespace vm::runtime
{
    /**
     * Executes exception handling opcodes
     * Handles TRY_BEGIN, TRY_END, CATCH, THROW, FINALLY
     *
     * Note: Most exception handling logic is implemented using C++ exceptions
     * in the VM's interpretLoop. These handlers primarily manage control flow
     * and marker opcodes.
     *
     * MYT-320: TRY_BEGIN/TRY_END/CATCH/FINALLY are no-op markers and are
     * inlined here (empty bodies); handleThrow stays out-of-line because it
     * needs ObjectInstance/ClassDefinition/SourceLocation and is exception-cold.
     */
    class VirtualMachine;

    class ExceptionExecutor
    {
    public:
        ExceptionExecutor(ExecutionContext& ctx, VirtualMachine* vmPtr)
            : context(ctx), vm(vmPtr) {}
        ~ExceptionExecutor() = default;

        inline void handleTryBegin(const bytecode::BytecodeProgram::Instruction& /*instr*/) {
            // TRY_BEGIN marks the start of a try block. Actual exception
            // handling is done in C++ try-catch in the VM's interpretLoop.
            // No-op here — the real work happens when THROW is executed.
        }

        inline void handleTryEnd(const bytecode::BytecodeProgram::Instruction& /*instr*/) {
            // TRY_END is never reached during normal execution because the
            // JUMP before it skips over it and goes directly to the CATCH
            // or FINALLY blocks. Marker only — no-op if hit.
        }

        inline void handleCatch(const bytecode::BytecodeProgram::Instruction& /*instr*/) {
            // CATCH should not be executed during normal flow. It is only
            // reached when an exception is thrown. The operand contains the
            // exception type index from constant pool. No-op during normal
            // execution; the catch block body follows this instruction.
        }

        inline void handleFinally(const bytecode::BytecodeProgram::Instruction& /*instr*/) {
            // FINALLY marks the start of a finally block. The finally block
            // code follows this instruction. Tracking of finally bounds is
            // done in VirtualMachine::interpretLoop. No-op here.
        }

        // Heavy handler: stays out-of-line in .cpp. Pulls in ObjectInstance /
        // ClassDefinition / SourceLocation / UserException and exception path
        // is cold by definition — no dispatch-density benefit.
        void handleThrow(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;
        VirtualMachine* vm;
    };
}
