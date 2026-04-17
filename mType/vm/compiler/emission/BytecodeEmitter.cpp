#include "BytecodeEmitter.hpp"
#include "../../../errors/ParseException.hpp"

namespace vm::compiler::emission
{
    BytecodeEmitter::BytecodeEmitter(bytecode::BytecodeProgram& program)
        : program(program)
    {
    }

    void BytecodeEmitter::emitWithLocation(bytecode::OpCode opcode, ast::ASTNode* node)
    {
        size_t offset = program.getCurrentOffset();
        program.emit(opcode);

        if (node) {
            const auto& loc = node->getLocation();
            program.addSourceLocation(offset, loc.getLine(), loc.getColumn(), loc.getFilename());
        }
    }

    void BytecodeEmitter::emitWithLocation(bytecode::OpCode opcode, uint64_t operand, ast::ASTNode* node)
    {
        size_t offset = program.getCurrentOffset();
        program.emit(opcode, operand);

        if (node) {
            const auto& loc = node->getLocation();
            program.addSourceLocation(offset, loc.getLine(), loc.getColumn(), loc.getFilename());
        }
    }

    void BytecodeEmitter::emitWithLocation(bytecode::OpCode opcode, uint64_t operand1, uint64_t operand2, ast::ASTNode* node)
    {
        size_t offset = program.getCurrentOffset();
        program.emit(opcode, operand1, operand2);

        if (node) {
            const auto& loc = node->getLocation();
            program.addSourceLocation(offset, loc.getLine(), loc.getColumn(), loc.getFilename());
        }
    }

    void BytecodeEmitter::emitWithLocation(bytecode::OpCode opcode, uint64_t operand1, uint64_t operand2, uint64_t operand3, ast::ASTNode* node)
    {
        size_t offset = program.getCurrentOffset();
        program.emit(opcode, std::vector<uint64_t>{operand1, operand2, operand3});

        if (node) {
            const auto& loc = node->getLocation();
            program.addSourceLocation(offset, loc.getLine(), loc.getColumn(), loc.getFilename());
        }
    }

    void BytecodeEmitter::emitWithLocation(bytecode::OpCode opcode, const std::vector<uint64_t>& operands, ast::ASTNode* node)
    {
        size_t offset = program.getCurrentOffset();
        program.emit(opcode, operands);

        if (node) {
            const auto& loc = node->getLocation();
            program.addSourceLocation(offset, loc.getLine(), loc.getColumn(), loc.getFilename());
        }
    }

    size_t BytecodeEmitter::emitJump(bytecode::OpCode jumpOp, ast::ASTNode* node)
    {
        size_t offset = program.getCurrentOffset();
        program.emit(jumpOp, 0xFFFFFFFFFFFFFFFF);  // Placeholder offset (64-bit)

        if (node) {
            const auto& loc = node->getLocation();
            program.addSourceLocation(offset, loc.getLine(), loc.getColumn(), loc.getFilename());
        }

        return program.getCurrentOffset() - 1;
    }

    void BytecodeEmitter::patchJump(size_t offset)
    {
        size_t jumpTarget = program.getCurrentOffset();
        program.patchJump(offset, static_cast<uint64_t>(jumpTarget));
    }

    void BytecodeEmitter::emitLoop(size_t loopStart, ast::ASTNode* node)
    {
        size_t offset = program.getCurrentOffset();
        program.emit(bytecode::OpCode::JUMP_BACK, static_cast<uint64_t>(loopStart));

        if (node) {
            const auto& loc = node->getLocation();
            program.addSourceLocation(offset, loc.getLine(), loc.getColumn(), loc.getFilename());
        }
    }

    bytecode::OpCode BytecodeEmitter::getBinaryOpCode(token::TokenType op, ArithmeticSpecialization spec)
    {
        switch (op) {
            case token::TokenType::PLUS:
                if (spec == ArithmeticSpecialization::INT) return bytecode::OpCode::ADD_INT;
                if (spec == ArithmeticSpecialization::FLOAT) return bytecode::OpCode::ADD_FLOAT;
                return bytecode::OpCode::ADD;
            case token::TokenType::MINUS:
                if (spec == ArithmeticSpecialization::INT) return bytecode::OpCode::SUB_INT;
                if (spec == ArithmeticSpecialization::FLOAT) return bytecode::OpCode::SUB_FLOAT;
                return bytecode::OpCode::SUB;
            case token::TokenType::MULTIPLY:
                if (spec == ArithmeticSpecialization::INT) return bytecode::OpCode::MUL_INT;
                if (spec == ArithmeticSpecialization::FLOAT) return bytecode::OpCode::MUL_FLOAT;
                return bytecode::OpCode::MUL;
            case token::TokenType::DIVIDE:
                if (spec == ArithmeticSpecialization::INT) return bytecode::OpCode::DIV_INT;
                if (spec == ArithmeticSpecialization::FLOAT) return bytecode::OpCode::DIV_FLOAT;
                return bytecode::OpCode::DIV;
            case token::TokenType::MODULO:
                return bytecode::OpCode::MOD;
            case token::TokenType::EQUALS:
                return bytecode::OpCode::EQ;
            case token::TokenType::NOT_EQUALS:
                return bytecode::OpCode::NE;
            case token::TokenType::LESS:
                return bytecode::OpCode::LT;
            case token::TokenType::GREATER:
                return bytecode::OpCode::GT;
            case token::TokenType::LESS_EQUALS:
                return bytecode::OpCode::LE;
            case token::TokenType::GREATER_EQUALS:
                return bytecode::OpCode::GE;
            case token::TokenType::AND:
                return bytecode::OpCode::AND;
            case token::TokenType::OR:
                return bytecode::OpCode::OR;
            case token::TokenType::BITWISE_AND:
                return bytecode::OpCode::BITWISE_AND_OP;
            case token::TokenType::BITWISE_OR:
                return bytecode::OpCode::BITWISE_OR_OP;
            case token::TokenType::BITWISE_XOR:
                return bytecode::OpCode::BITWISE_XOR_OP;
            case token::TokenType::LEFT_SHIFT:
                return bytecode::OpCode::LEFT_SHIFT_OP;
            case token::TokenType::RIGHT_SHIFT:
                return bytecode::OpCode::RIGHT_SHIFT_OP;
            default:
                throw errors::ParseException("Unsupported binary operator");
        }
    }

    bytecode::OpCode BytecodeEmitter::getUnaryOpCode(token::TokenType op)
    {
        switch (op) {
            case token::TokenType::MINUS:
                return bytecode::OpCode::NEG;
            case token::TokenType::NOT:
                return bytecode::OpCode::NOT;
            case token::TokenType::INCREMENT:
                return bytecode::OpCode::INC;
            case token::TokenType::DECREMENT:
                return bytecode::OpCode::DEC;
            case token::TokenType::BITWISE_NOT:
                return bytecode::OpCode::BITWISE_NOT_OP;
            default:
                throw errors::ParseException("Unsupported unary operator");
        }
    }
}
