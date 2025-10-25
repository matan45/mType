#pragma once
#include "CompilerContext.hpp"
#include "MethodCompilerHelper.hpp"
#include "ParameterValidator.hpp"
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
#include <memory>

namespace vm::compiler::visitors
{
    /**
     * Compiles class-related nodes to bytecode
     * Handles: classes, fields, object creation, member access, method calls
     * Delegates method/constructor compilation to MethodCompilerHelper
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
        value::Value compileSuperMemberAccess(ast::SuperMemberAccessNode* node);
        value::Value compileSuperMemberAssignment(ast::SuperMemberAssignmentNode* node);

    private:
        CompilerContext& ctx;
        std::unique_ptr<MethodCompilerHelper> methodHelper;
        std::unique_ptr<ParameterValidator> paramValidator;

        // Helper methods for compileNew
        std::vector<std::string> parseAndValidateGenericTypeArguments(const std::string& fullClassName,
                                                                       const ast::SourceLocation& location);
        void emitNewObjectBytecode(ast::NewNode* node, const std::string& fullClassName);
    };
}
