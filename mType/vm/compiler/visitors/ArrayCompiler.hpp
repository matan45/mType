#pragma once
#include "CompilerContext.hpp"
#include "../../../ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../../../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../ast/nodes/statements/IndexAssignmentNode.hpp"
#include "../../../value/ValueType.hpp"

namespace vm::compiler::visitors
{
    /**
     * Compiles array operations to bytecode
     * Handles: array creation, array literals, index access, index assignment
     */
    class ArrayCompiler
    {
    public:
        explicit ArrayCompiler(CompilerContext& context);
        ~ArrayCompiler() = default;

        // Array compilation methods
        value::Value compileArrayCreation(ast::ArrayCreationNode* node);
        value::Value compileArrayLiteral(ast::ArrayLiteralNode* node);
        value::Value compileIndexAccess(ast::IndexAccessNode* node);
        value::Value compileIndexAssignment(ast::IndexAssignmentNode* node);

    private:
        CompilerContext& ctx;
    };
}
