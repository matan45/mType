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

        // String building (pops count segments, concatenates, pushes result)
        void handleStringBuild(size_t count);

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

        // Helper to format multi-dimensional array slice recursively
        template<typename ArrayType>
        void formatMultiArraySlice(const ArrayType& arr, const std::vector<size_t>& dims,
                                   size_t dimIndex, size_t offset, std::string& out) const;
    };
}
