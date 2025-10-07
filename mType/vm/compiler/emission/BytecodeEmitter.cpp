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

    void BytecodeEmitter::emitWithLocation(bytecode::OpCode opcode, uint32_t operand, ast::ASTNode* node)
    {
        size_t offset = program.getCurrentOffset();
        program.emit(opcode, operand);

        if (node) {
            const auto& loc = node->getLocation();
            program.addSourceLocation(offset, loc.getLine(), loc.getColumn(), loc.getFilename());
        }
    }

    size_t BytecodeEmitter::emitJump(bytecode::OpCode jumpOp)
    {
        program.emit(jumpOp, 0xFFFFFFFF);  // Placeholder offset
        return program.getCurrentOffset() - 1;
    }

    void BytecodeEmitter::patchJump(size_t offset)
    {
        size_t jumpTarget = program.getCurrentOffset();
        program.patchJump(offset, static_cast<uint32_t>(jumpTarget));
    }

    void BytecodeEmitter::emitLoop(size_t loopStart)
    {
        program.emit(bytecode::OpCode::JUMP_BACK, static_cast<uint32_t>(loopStart));
    }

    bytecode::OpCode BytecodeEmitter::getBinaryOpCode(token::TokenType op, bool typeSpecialized)
    {
        switch (op) {
            case token::TokenType::PLUS:
                return typeSpecialized ? bytecode::OpCode::ADD_INT : bytecode::OpCode::ADD;
            case token::TokenType::MINUS:
                return typeSpecialized ? bytecode::OpCode::SUB_INT : bytecode::OpCode::SUB;
            case token::TokenType::MULTIPLY:
                return typeSpecialized ? bytecode::OpCode::MUL_INT : bytecode::OpCode::MUL;
            case token::TokenType::DIVIDE:
                return typeSpecialized ? bytecode::OpCode::DIV_INT : bytecode::OpCode::DIV;
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
            default:
                throw errors::ParseException("Unsupported unary operator");
        }
    }
}
