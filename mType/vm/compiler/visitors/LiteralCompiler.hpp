#pragma once
#include "CompilerContext.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../value/ValueType.hpp"

namespace vm::compiler::visitors
{
    /**
     * Compiles literal value nodes to bytecode
     * Handles: int, float, string, bool, null
     */
    class LiteralCompiler
    {
    public:
        explicit LiteralCompiler(CompilerContext& context);
        ~LiteralCompiler() = default;

        // Literal compilation methods
        value::Value compileInteger(ast::IntegerNode* node);
        value::Value compileFloat(ast::FloatNode* node);
        value::Value compileString(ast::StringNode* node);
        value::Value compileBool(ast::BoolNode* node);
        value::Value compileNull(ast::NullNode* node);

    private:
        CompilerContext& ctx;
    };
}
