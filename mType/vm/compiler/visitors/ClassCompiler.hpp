#pragma once
#include "CompilerContext.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../../ast/nodes/classes/SuperConstructorCallNode.hpp"
#include "../../../ast/nodes/classes/SuperMethodCallNode.hpp"
#include "../../../value/ValueType.hpp"

namespace vm::compiler::visitors
{
    /**
     * Compiles class-related nodes to bytecode
     * Handles: classes, methods, constructors, fields, object creation, member access, method calls
     */
    class ClassCompiler
    {
    public:
        explicit ClassCompiler(CompilerContext& context);
        ~ClassCompiler() = default;

        // Class compilation methods
        value::Value compileClass(ast::ClassNode* node);
        value::Value compileMethod(ast::MethodNode* node);
        value::Value compileConstructor(ast::ConstructorNode* node);
        value::Value compileField(ast::FieldNode* node);
        value::Value compileNew(ast::NewNode* node);
        value::Value compileMemberAccess(ast::MemberAccessNode* node);
        value::Value compileMemberAssignment(ast::MemberAssignmentNode* node);
        value::Value compileMethodCall(ast::MethodCallNode* node);
        value::Value compileSuperConstructorCall(ast::SuperConstructorCallNode* node);
        value::Value compileSuperMethodCall(ast::SuperMethodCallNode* node);

    private:
        CompilerContext& ctx;

        // Helper methods
        void compileDefaultConstructor(ast::ClassNode* node);
        void initializeInstanceFields(ast::ClassNode* node);
    };
}
