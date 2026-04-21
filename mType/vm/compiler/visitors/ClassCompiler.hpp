#pragma once
#include "CompilerContext.hpp"
#include "MethodCompilerHelper.hpp"
#include "ParameterValidator.hpp"
#include "../overload/OverloadResolutionHelper.hpp"
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
#include "../../../runtimeTypes/klass/ConstructorDefinition.hpp"
#include <memory>
#include <unordered_map>

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
        value::Value compileThisConstructorCall(ast::ThisConstructorCallNode* node);
        value::Value compileSuperMethodCall(ast::SuperMethodCallNode* node);
        value::Value compileSuperMemberAccess(ast::SuperMemberAccessNode* node);
        value::Value compileSuperMemberAssignment(ast::SuperMemberAssignmentNode* node);

    private:
        CompilerContext& ctx;
        std::unique_ptr<MethodCompilerHelper> methodHelper;
        std::unique_ptr<ParameterValidator> paramValidator;
        std::unique_ptr<overload::OverloadResolutionHelper> overloadResolver;

        // Helper methods for compileNew
        std::vector<std::string> parseAndValidateGenericTypeArguments(const std::string& fullClassName,
                                                                       const ast::SourceLocation& location);
        void emitNewObjectBytecode(ast::NewNode* node, const std::string& fullClassName,
                                  const runtimeTypes::klass::ConstructorDefinition* constructor,
                                  const std::unordered_map<std::string, std::string>& genericTypeBindings);

        // compileMethodCall helpers
        void compileStaticMethodCall(ast::MethodCallNode* node);
        void compileInstanceMethodCall(ast::MethodCallNode* node);

        // Helper method for auto-boxing method arguments
        bool tryAutoBoxArgument(ast::ASTNode* argument, const std::string& expectedType);

        // Check if receiver expression is known to be non-nullable
        bool isReceiverNonNullable(ast::ASTNode* receiverNode);

        // Phase 2 (allocation perf): intersect definite-assignment sets across
        // all constructors of the class and push the result onto ClassDefinition.
        void computeSkipDefaultInitFields(ast::ClassNode* node);

        // Phase 3 (allocation perf): flag trivial constructors for the
        // VM's fast-path copy-args-into-fields path.
        void markTrivialConstructors(ast::ClassNode* node);
    };
}
