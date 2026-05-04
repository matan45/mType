#pragma once
#include "CompilerContext.hpp"
#include <cstddef>
#include "../../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/ProgramNode.hpp"
#include "../../../ast/nodes/statements/IfNode.hpp"
#include "../../../ast/nodes/statements/WhileNode.hpp"
#include "../../../ast/nodes/statements/ForNode.hpp"
#include "../../../ast/nodes/statements/TryNode.hpp"
#include "../../../ast/nodes/statements/CatchNode.hpp"
#include "../../../ast/nodes/functions/ReturnNode.hpp"
#include "../../../value/ValueType.hpp"
#include <vector>

namespace vm::compiler::visitors
{
    /**
     * Compiles statement nodes to bytecode
     * Handles: declarations, assignments, blocks, programs
     */
    class StatementCompiler
    {
    public:
        explicit StatementCompiler(CompilerContext& context);
        ~StatementCompiler() = default;

        // Statement compilation methods
        value::Value compileAssignment(ast::AssignmentNode* node);
        value::Value compileBlock(ast::BlockNode* node);
        value::Value compileProgram(ast::ProgramNode* node);

    private:
        CompilerContext& ctx;

        // Helper methods for compileAssignment
        bool detectReassignment(const std::string& name, std::string& existingClassName);
        void validateClassOrInterfaceType(ast::AssignmentNode* node);
        void validateLambdaAssignment(ast::AssignmentNode* node, bool isReassignment,
                                     const std::string& existingClassName);
        void validateReassignmentType(ast::AssignmentNode* node, const std::string& existingClassName);
        void emitVariableDeclaration(ast::AssignmentNode* node);
        void emitVariableReassignment(ast::AssignmentNode* node, bool isReassignment);

        // Helper for lambda return type validation
        std::vector<ast::nodes::functions::ReturnNode*> collectReturnStatements(ast::ASTNode* node);

        // Phase 4: Auto-boxing helper
        // Returns true if auto-boxing was applied and bytecode was emitted
        bool tryEmitAutoBoxing(ast::ASTNode* valueNode, const std::string& targetClassName);

        // Check if a statement node is an expression that leaves a value on the stack
        bool isExpressionStatement(ast::ASTNode* node) const;

        // Emit the trailing POP needed to keep the operand stack clean after a
        // statement, covering two cases: (1) expression statements flagged by
        // `isExpressionStatement`, (2) statements whose last emitted opcode is
        // STORE_LOCAL — the runtime's handleStoreLocal re-pushes the stored
        // value, and for local vars used as statements nothing consumes it.
        // `offsetBefore` is `ctx.program.getCurrentOffset()` captured before
        // the statement's accept() call.
        void emitStatementCleanup(ast::ASTNode* stmt, size_t offsetBefore);
    };
}
