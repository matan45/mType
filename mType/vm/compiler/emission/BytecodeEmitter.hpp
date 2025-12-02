#pragma once
#include "../../bytecode/BytecodeProgram.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../ast/ASTNode.hpp"
#include "../../../token/TokenType.hpp"
#include <cstddef>
#include <cstdint>

namespace vm::compiler::emission
{
    /**
     * Handles bytecode emission and jump management
     * Provides helper methods for emitting instructions with source location tracking
     */
    class BytecodeEmitter
    {
    public:
        explicit BytecodeEmitter(bytecode::BytecodeProgram& program);
        ~BytecodeEmitter() = default;

        // Bytecode emission with source location tracking
        void emitWithLocation(bytecode::OpCode opcode, ast::ASTNode* node);
        void emitWithLocation(bytecode::OpCode opcode, uint64_t operand, ast::ASTNode* node);
        void emitWithLocation(bytecode::OpCode opcode, uint64_t operand1, uint64_t operand2, ast::ASTNode* node);
        void emitWithLocation(bytecode::OpCode opcode, uint64_t operand1, uint64_t operand2, uint64_t operand3, ast::ASTNode* node);
        void emitWithLocation(bytecode::OpCode opcode, const std::vector<uint64_t>& operands, ast::ASTNode* node);

        // Jump management
        size_t emitJump(bytecode::OpCode jumpOp, ast::ASTNode* node = nullptr);
        void patchJump(size_t offset);
        void emitLoop(size_t loopStart, ast::ASTNode* node = nullptr);

        // OpCode conversion helpers
        bytecode::OpCode getBinaryOpCode(token::TokenType op, bool typeSpecialized = false);
        bytecode::OpCode getUnaryOpCode(token::TokenType op);

    private:
        bytecode::BytecodeProgram& program;
    };
}
