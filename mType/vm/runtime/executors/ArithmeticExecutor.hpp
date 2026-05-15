#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/CheckedArithmetic.hpp"
#include "../../../value/ValueShim.hpp"

namespace vm::runtime
{
    /**
     * Executes arithmetic opcodes
     * Handles ADD, SUB, MUL, DIV, MOD, NEG, INC, DEC and optimized integer operations
     *
     * MYT-319: hot dispatch handlers live in the header so MSVC v145 (no /GL)
     * can inline them through the dispatch switch's unique_ptr deref. The
     * heavy generic performBinaryOp / valueToString / handleStringBuild /
     * formatMultiArraySlice helpers stay in ArithmeticExecutor.cpp because
     * they pull in ObjectInstance, NativeArray, FlatMultiArray, SparseMultiArray,
     * and StringPool — no per-dispatch benefit to inlining them.
     */
    class ArithmeticExecutor
    {
    public:
        explicit ArithmeticExecutor(ExecutionContext& ctx) : context(ctx) {}
        ~ArithmeticExecutor() = default;

        // Binary arithmetic operations — generic path goes through performBinaryOp (out-of-line).
        inline void handleAdd() {
            value::Value right = context.stackManager->pop();
            value::Value left = context.stackManager->pop();
            context.stackManager->push(performBinaryOp(left, right, bytecode::OpCode::ADD));
        }

        inline void handleSub() {
            value::Value right = context.stackManager->pop();
            value::Value left = context.stackManager->pop();
            context.stackManager->push(performBinaryOp(left, right, bytecode::OpCode::SUB));
        }

        inline void handleMul() {
            value::Value right = context.stackManager->pop();
            value::Value left = context.stackManager->pop();
            context.stackManager->push(performBinaryOp(left, right, bytecode::OpCode::MUL));
        }

        inline void handleDiv() {
            value::Value right = context.stackManager->pop();
            value::Value left = context.stackManager->pop();
            context.stackManager->push(performBinaryOp(left, right, bytecode::OpCode::DIV));
        }

        inline void handleMod() {
            value::Value right = context.stackManager->pop();
            value::Value left = context.stackManager->pop();
            context.stackManager->push(performBinaryOp(left, right, bytecode::OpCode::MOD));
        }

        // Unary arithmetic operations
        inline void handleNeg() {
            // MYT-318: replaceTop avoids the pop+push round trip; we read through
            // a reference to TOS and overwrite in place.
            if (context.stackManager->empty()) {
                throw errors::RuntimeException("Stack underflow: NEG requires 1 value");
            }
            const auto& val = context.stackManager->peekRef(0);
            if (value::isInt(val)) {
                context.stackManager->replaceTop(utils::wrappingNeg64(value::asInt(val)));
            } else if (value::isFloat(val)) {
                context.stackManager->replaceTop(-value::asFloat(val));
            } else {
                throw errors::RuntimeException("NEG requires numeric value");
            }
        }

        inline void handleInc() {
            if (context.stackManager->empty()) {
                throw errors::RuntimeException("Stack underflow: INC requires 1 value");
            }
            const auto& val = context.stackManager->peekRef(0);
            if (value::isInt(val)) {
                context.stackManager->replaceTop(utils::wrappingAdd64(value::asInt(val), 1));
            } else if (value::isFloat(val)) {
                context.stackManager->replaceTop(value::asFloat(val) + 1.0);
            } else {
                throw errors::RuntimeException("INC requires numeric value");
            }
        }

        inline void handleDec() {
            if (context.stackManager->empty()) {
                throw errors::RuntimeException("Stack underflow: DEC requires 1 value");
            }
            const auto& val = context.stackManager->peekRef(0);
            if (value::isInt(val)) {
                context.stackManager->replaceTop(utils::wrappingSub64(value::asInt(val), 1));
            } else if (value::isFloat(val)) {
                context.stackManager->replaceTop(value::asFloat(val) - 1.0);
            } else {
                throw errors::RuntimeException("DEC requires numeric value");
            }
        }

        // Optimized integer operations
        inline void handleAddInt() {
            if (context.stackManager->size() < 2) {
                throw errors::RuntimeException("Stack underflow: ADD_INT requires 2 values");
            }
            // Phase 6: Type guard — deopt to generic ADD if types don't match
            const auto& right = context.stackManager->peekRef(0);
            const auto& left = context.stackManager->peekRef(1);
            if (!value::isInt(left) || !value::isInt(right)) {
                handleAdd(); // Fall back to generic
                return;
            }
            // MYT-318: pop+pop+push collapses to a single size mutation via
            // binaryReplaceTop — value extraction reads through the refs above.
            int64_t r = value::asInt(right);
            int64_t l = value::asInt(left);
            context.stackManager->binaryReplaceTop(utils::wrappingAdd64(l, r));
        }

        // MYT-198: fused PUSH_INT + ADD_INT. Reads the integer literal from the
        // constant pool via state.fusedSlot (populated at fusion time with the
        // original PUSH_INT's operand[0]), adds it to the integer already on
        // tos, and pushes the result. Deopts to handleAdd on non-int tos.
        // MYT-201: `state` is the per-IP cached state; dispatch loop looks it
        // up once and hands the reference down.
        inline void handleAddIntConst(
            const bytecode::BytecodeProgram::Instruction& /*instr*/,
            const bytecode::BytecodeProgram::CachedInstructionState& state) {
            // MYT-198: fused PUSH_INT + ADD_INT. At entry, tos is the left operand
            // (the value that was on stack before the NOPed PUSH_INT would have
            // pushed its literal). The literal is in the constant pool at
            // state.fusedSlot — set by tryFuseAddIntConst.
            if (context.stackManager->size() < 1) {
                throw errors::RuntimeException("Stack underflow: ADD_INT_CONST requires 1 value");
            }
            const auto& tos = context.stackManager->peek(0);
            if (!value::isInt(tos)) {
                // Un-fuse on type miss: restore PUSH_INT + ADD_INT at the pair, push
                // the literal so the stack matches the pre-fusion shape, bump
                // fusedDeoptCount sticky.
                int64_t literal = context.program->getConstantPool().getInteger(state.fusedSlot);
                context.stackManager->push(literal);
                const size_t ip = context.instructionPointer;
                auto& mut = context.getMutableInstructionAt(ip);
                auto& prevMut = context.getMutableInstructionAt(ip - 1);
                prevMut.opcode = bytecode::OpCode::PUSH_INT;
                // MYT-201: fused state now lives on the side table.
                auto& mutState = context.getOrCreateCachedState(ip);
                prevMut.setSingleOperand(static_cast<uint64_t>(mutState.fusedSlot));
                mut.opcode = bytecode::OpCode::ADD_INT;
                if (mutState.fusedDeoptCount < 255) ++mutState.fusedDeoptCount;
                handleAdd();
                return;
            }
            int64_t literal = context.program->getConstantPool().getInteger(state.fusedSlot);
            int64_t l = value::asInt(context.stackManager->peekRef(0));
            context.stackManager->replaceTop(utils::wrappingAdd64(l, literal));
        }

        inline void handleSubInt() {
            if (context.stackManager->size() < 2) {
                throw errors::RuntimeException("Stack underflow: SUB_INT requires 2 values");
            }
            const auto& right = context.stackManager->peekRef(0);
            const auto& left = context.stackManager->peekRef(1);
            if (!value::isInt(left) || !value::isInt(right)) {
                handleSub();
                return;
            }
            int64_t r = value::asInt(right);
            int64_t l = value::asInt(left);
            context.stackManager->binaryReplaceTop(utils::wrappingSub64(l, r));
        }

        inline void handleMulInt() {
            if (context.stackManager->size() < 2) {
                throw errors::RuntimeException("Stack underflow: MUL_INT requires 2 values");
            }
            const auto& right = context.stackManager->peekRef(0);
            const auto& left = context.stackManager->peekRef(1);
            if (!value::isInt(left) || !value::isInt(right)) {
                handleMul();
                return;
            }
            int64_t r = value::asInt(right);
            int64_t l = value::asInt(left);
            context.stackManager->binaryReplaceTop(utils::wrappingMul64(l, r));
        }

        inline void handleDivInt() {
            if (context.stackManager->size() < 2) {
                throw errors::RuntimeException("Stack underflow: DIV_INT requires 2 values");
            }
            const auto& right = context.stackManager->peekRef(0);
            const auto& left = context.stackManager->peekRef(1);
            if (!value::isInt(left) || !value::isInt(right)) {
                handleDiv();
                return;
            }
            int64_t r = value::asInt(right);
            int64_t l = value::asInt(left);
            if (r == 0) {
                utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero");
            }
            context.stackManager->binaryReplaceTop(l / r);
        }

        // Optimized float operations
        inline void handleAddFloat() {
            if (context.stackManager->size() < 2) {
                throw errors::RuntimeException("Stack underflow: ADD_FLOAT requires 2 values");
            }
            const auto& right = context.stackManager->peekRef(0);
            const auto& left = context.stackManager->peekRef(1);
            if (!value::isFloat(left) || !value::isFloat(right)) {
                handleAdd();
                return;
            }
            double r = value::asFloat(right);
            double l = value::asFloat(left);
            context.stackManager->binaryReplaceTop(l + r);
        }

        inline void handleSubFloat() {
            if (context.stackManager->size() < 2) {
                throw errors::RuntimeException("Stack underflow: SUB_FLOAT requires 2 values");
            }
            const auto& right = context.stackManager->peekRef(0);
            const auto& left = context.stackManager->peekRef(1);
            if (!value::isFloat(left) || !value::isFloat(right)) {
                handleSub();
                return;
            }
            double r = value::asFloat(right);
            double l = value::asFloat(left);
            context.stackManager->binaryReplaceTop(l - r);
        }

        inline void handleMulFloat() {
            if (context.stackManager->size() < 2) {
                throw errors::RuntimeException("Stack underflow: MUL_FLOAT requires 2 values");
            }
            const auto& right = context.stackManager->peekRef(0);
            const auto& left = context.stackManager->peekRef(1);
            if (!value::isFloat(left) || !value::isFloat(right)) {
                handleMul();
                return;
            }
            double r = value::asFloat(right);
            double l = value::asFloat(left);
            context.stackManager->binaryReplaceTop(l * r);
        }

        inline void handleDivFloat() {
            if (context.stackManager->size() < 2) {
                throw errors::RuntimeException("Stack underflow: DIV_FLOAT requires 2 values");
            }
            const auto& right = context.stackManager->peekRef(0);
            const auto& left = context.stackManager->peekRef(1);
            if (!value::isFloat(left) || !value::isFloat(right)) {
                handleDiv();
                return;
            }
            double r = value::asFloat(right);
            double l = value::asFloat(left);
            if (r == 0.0) {
                utils::ErrorLocationHelper::throwRuntimeError(context, "Division by zero");
            }
            context.stackManager->binaryReplaceTop(l / r);
        }

        // Heavy out-of-line helpers — defined in ArithmeticExecutor.cpp.
        void handleStringBuild(size_t count);

    private:
        ExecutionContext& context;

        // Out-of-line: deep dependencies on ObjectInstance / NativeArray /
        // FlatMultiArray / SparseMultiArray / StringPool / ValueObject.
        value::Value performBinaryOp(const value::Value& left, const value::Value& right, bytecode::OpCode op);
        std::string valueToString(const value::Value& val) const;

        // Helper to format multi-dimensional array slice recursively
        template<typename ArrayType>
        void formatMultiArraySlice(const ArrayType& arr, const std::vector<size_t>& dims,
                                   size_t dimIndex, size_t offset, std::string& out) const;
    };
}
