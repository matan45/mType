#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../bytecode/OpCode.hpp"

namespace vm::runtime
{
    /**
     * Executes arithmetic opcodes
     * Handles ADD, SUB, MUL, DIV, MOD, NEG, INC, DEC and optimized integer operations
     */
    class ArithmeticExecutor
    {
    public:
        explicit ArithmeticExecutor(ExecutionContext& ctx);
        ~ArithmeticExecutor() = default;

        // Binary arithmetic operations
        void handleAdd();
        void handleSub();
        void handleMul();
        void handleDiv();
        void handleMod();

        // Unary arithmetic operations
        void handleNeg();
        void handleInc();
        void handleDec();

        // Optimized integer operations
        void handleAddInt();
        void handleSubInt();
        void handleMulInt();
        void handleDivInt();

    private:
        ExecutionContext& context;

        // Helper method for binary operations
        value::Value performBinaryOp(const value::Value& left, const value::Value& right, bytecode::OpCode op);

        // Helper to convert value to string (for string concatenation)
        std::string valueToString(const value::Value& val) const;
    };
}
