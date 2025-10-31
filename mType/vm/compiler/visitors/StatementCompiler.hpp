#pragma once
#include "CompilerContext.hpp"
#include "../../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/ProgramNode.hpp"
#include "../../../value/ValueType.hpp"

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
    };
}
