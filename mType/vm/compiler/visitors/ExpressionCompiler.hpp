#pragma once
#include "CompilerContext.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/expressions/CastExpression.hpp"
#include "../../../ast/nodes/expressions/InstanceOfExpression.hpp"
#include "../../../value/ValueType.hpp"

namespace vm::compiler::visitors
{
    // Forward declarations
    class LiteralCompiler;
    class ArrayCompiler;

    /**
     * Compiles expression nodes to bytecode
     * Handles: binary ops, unary ops, ternary ops, variables, casts, instanceof
     */
    class ExpressionCompiler
    {
    public:
        ExpressionCompiler(CompilerContext& context,
                          LiteralCompiler& literalComp,
                          ArrayCompiler& arrayComp);
        ~ExpressionCompiler() = default;

        // Expression compilation methods
        value::Value compileBinaryOp(ast::BinaryOpNode* node);
        value::Value compileUnaryOp(ast::UnaryOpNode* node);
        value::Value compileTernaryOp(ast::TernaryOpNode* node);
        value::Value compileVariable(ast::VariableNode* node);
        value::Value compileCast(ast::CastExpression* node);
        value::Value compileInstanceOf(ast::InstanceOfExpression* node);

    private:
        CompilerContext& ctx;
        LiteralCompiler& literalCompiler;
        ArrayCompiler& arrayCompiler;

        // Phase 4: Operator overloading helper
        // Returns true if operator overloading was applied, false otherwise
        bool tryEmitOperatorOverloading(ast::BinaryOpNode* node,
                                       ast::ASTNode* left,
                                       ast::ASTNode* right,
                                       token::TokenType op);

        // String interpolation optimization: flatten PLUS chains into STRING_BUILD
        bool tryEmitStringBuild(ast::BinaryOpNode* node);
        void flattenStringConcat(ast::ASTNode* node, std::vector<ast::ASTNode*>& segments);
    };
}
