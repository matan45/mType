#include "LiteralCompiler.hpp"
#include <cstdint>
#include "../../bytecode/OpCode.hpp"

namespace vm::compiler::visitors
{
    LiteralCompiler::LiteralCompiler(CompilerContext& context)
        : ctx(context)
    {
    }

    value::Value LiteralCompiler::compileInteger(ast::IntegerNode* node)
    {
        size_t index = ctx.program.getConstantPool().addInteger(node->getValue());
        ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_INT, static_cast<uint64_t>(index), node);
        return std::monostate{};
    }

    value::Value LiteralCompiler::compileFloat(ast::FloatNode* node)
    {
        size_t index = ctx.program.getConstantPool().addFloat(node->getValue());
        ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_FLOAT, static_cast<uint64_t>(index), node);
        return std::monostate{};
    }

    value::Value LiteralCompiler::compileString(ast::StringNode* node)
    {
        size_t index = ctx.program.getConstantPool().addString(node->getValue());
        ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_STRING, static_cast<uint64_t>(index), node);
        return std::monostate{};
    }

    value::Value LiteralCompiler::compileBool(ast::BoolNode* node)
    {
        ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_BOOL, node->getValue() ? 1 : 0, node);
        return std::monostate{};
    }

    value::Value LiteralCompiler::compileNull(ast::NullNode* node)
    {
        ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
        return std::monostate{};
    }
}
