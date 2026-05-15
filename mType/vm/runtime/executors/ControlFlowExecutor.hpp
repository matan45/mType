#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../utils/NullCheckUtils.hpp"

// Forward declaration for OSR
namespace vm::jit { class OSRManager; }

namespace vm::runtime
{
    /**
     * Executes control flow opcodes
     * Handles JUMP, JUMP_IF_FALSE, JUMP_IF_TRUE, JUMP_BACK, RETURN, RETURN_VALUE
     *
     * MYT-319: simple jump variants are inlined in the header so MSVC v145
     * (no /GL) can inline them through the dispatch switch's unique_ptr deref.
     * handleJumpBack/Return/ReturnValue stay out-of-line — they pull in OSR,
     * debug, profiler, Promise, and ObjectInstance machinery, which would
     * bloat every TU that includes this header for no extra dispatch benefit
     * (Return paths are one-per-call, not one-per-loop-iteration).
     */
    class ControlFlowExecutor
    {
    public:
        explicit ControlFlowExecutor(ExecutionContext& ctx) : context(ctx) {}
        ~ControlFlowExecutor() = default;

        // MYT-318: JUMP* operand-count contract is enforced by
        // BytecodeProgram::validateInstructionOperands at program load.

        inline void handleJump(const bytecode::BytecodeProgram::Instruction& instr) {
            context.instructionPointer = instr.inlineOperands[0] - 1;  // -1 because loop increments
        }

        inline void handleJumpIfFalse(const bytecode::BytecodeProgram::Instruction& instr) {
            value::Value condition = context.stackManager->pop();
            if (!isTruthy(condition)) {
                context.instructionPointer = instr.inlineOperands[0] - 1;
            }
        }

        inline void handleJumpIfTrue(const bytecode::BytecodeProgram::Instruction& instr) {
            value::Value condition = context.stackManager->pop();
            if (isTruthy(condition)) {
                context.instructionPointer = instr.inlineOperands[0] - 1;
            }
        }

        inline void handleJumpIfFalseOrPop(const bytecode::BytecodeProgram::Instruction& instr) {
            // Peek at the value without popping
            value::Value condition = context.stackManager->peek();
            if (!isTruthy(condition)) {
                // If false, jump (keeping the false value on stack as result)
                context.instructionPointer = instr.inlineOperands[0] - 1;
            } else {
                // If true, pop it and continue to evaluate the right side
                context.stackManager->pop();
            }
        }

        inline void handleJumpIfTrueOrPop(const bytecode::BytecodeProgram::Instruction& instr) {
            // Peek at the value without popping
            value::Value condition = context.stackManager->peek();
            if (isTruthy(condition)) {
                // If true, jump (keeping the true value on stack as result)
                context.instructionPointer = instr.inlineOperands[0] - 1;
            } else {
                // If false, pop it and continue to evaluate the right side
                context.stackManager->pop();
            }
        }

        inline void handleJumpIfNull(const bytecode::BytecodeProgram::Instruction& instr) {
            value::Value val = context.stackManager->pop();
            if (utils::isNullValue(val)) {
                context.instructionPointer = instr.inlineOperands[0] - 1;
            }
        }

        // Heavyweight handlers — defined out-of-line in ControlFlowExecutor.cpp.
        // See header comment above for rationale.
        void handleJumpBack(const bytecode::BytecodeProgram::Instruction& instr);
        void handleReturn();
        void handleReturnValue();

        // Phase 5: OSR integration
        void setOSRManager(vm::jit::OSRManager* manager) { osrManager = manager; }

    private:
        ExecutionContext& context;
        vm::jit::OSRManager* osrManager = nullptr;

        // Helper method for truthiness evaluation
        inline bool isTruthy(const value::Value& val) const {
            return utils::isTruthy(val);
        }
    };
}
