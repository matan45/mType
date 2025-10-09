#pragma once
#include "../../ast/ASTVisitor.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../../environment/Environment.hpp"
#include "../../token/TokenType.hpp"
#include "emission/BytecodeEmitter.hpp"
#include "variables/VariableTracker.hpp"
#include "variables/GlobalVariableRegistry.hpp"
#include "variables/FunctionFrameManager.hpp"
#include "control/LoopContextManager.hpp"
#include "control/SwitchContextManager.hpp"
#include "types/TypeInferenceEngine.hpp"
#include "types/TypeValidator.hpp"
#include "types/GenericTypeResolver.hpp"
#include "registration/ClassRegistrar.hpp"
#include "registration/InterfaceRegistrar.hpp"
#include "visitors/LiteralCompiler.hpp"
#include "visitors/ArrayCompiler.hpp"
#include "visitors/ExpressionCompiler.hpp"
#include "visitors/StatementCompiler.hpp"
#include "visitors/ControlFlowCompiler.hpp"
#include "visitors/FunctionCompiler.hpp"
#include "visitors/ClassCompiler.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace vm::compiler
{
    /**
     * Compiles AST nodes to bytecode instructions
     * Coordinates specialized compiler components following SOLID principles
     * Implements the visitor pattern to traverse the AST
     */
    class BytecodeCompiler : public ast::ASTVisitor<value::Value>
    {
    public:
        explicit BytecodeCompiler(std::shared_ptr<environment::Environment> env);
        ~BytecodeCompiler() = default;

        // Main compilation entry point
        bytecode::BytecodeProgram compile(ast::ASTNode* root);

        // ASTVisitor interface implementation (returns Value but we discard it)
        value::Value visitProgramNode(ast::ProgramNode* node) override;
        value::Value visitBlockNode(ast::BlockNode* node) override;

        // Literals
        value::Value visitFloatNode(ast::FloatNode* node) override;
        value::Value visitIntegerNode(ast::IntegerNode* node) override;
        value::Value visitStringNode(ast::StringNode* node) override;
        value::Value visitBoolNode(ast::BoolNode* node) override;
        value::Value visitNullNode(ast::NullNode* node) override;

        // Variables
        value::Value visitVariableNode(ast::VariableNode* node) override;
        value::Value visitDeclarationNode(ast::DeclarationNode* node) override;
        value::Value visitAssignmentNode(ast::AssignmentNode* node) override;

        // Operators
        value::Value visitBinaryOpNode(ast::BinaryOpNode* node) override;
        value::Value visitTernaryOpNode(ast::TernaryOpNode* node) override;
        value::Value visitUnaryOpNode(ast::UnaryOpNode* node) override;

        // Control flow
        value::Value visitIfNode(ast::IfNode* node) override;
        value::Value visitWhileNode(ast::WhileNode* node) override;
        value::Value visitDoWhileNode(ast::DoWhileNode* node) override;
        value::Value visitForNode(ast::ForNode* node) override;
        value::Value visitForEachNode(ast::ForEachNode* node) override;
        value::Value visitBreakNode(ast::BreakNode* node) override;
        value::Value visitContinueNode(ast::ContinueNode* node) override;
        value::Value visitSwitchNode(ast::SwitchNode* node) override;
        value::Value visitCaseNode(ast::CaseNode* node) override;
        value::Value visitDefaultCaseNode(ast::DefaultCaseNode* node) override;

        // Functions
        value::Value visitFunctionNode(ast::FunctionNode* node) override;
        value::Value visitFunctionCallNode(ast::FunctionCallNode* node) override;
        value::Value visitReturnNode(ast::ReturnNode* node) override;
        value::Value visitNativeFunctionNode(ast::NativeFunctionNode* node) override;

        // Classes and Objects
        value::Value visitClassNode(ast::ClassNode* node) override;
        value::Value visitMethodNode(ast::MethodNode* node) override;
        value::Value visitConstructorNode(ast::ConstructorNode* node) override;
        value::Value visitFieldNode(ast::FieldNode* node) override;
        value::Value visitNewNode(ast::NewNode* node) override;
        value::Value visitMemberAccessNode(ast::MemberAccessNode* node) override;
        value::Value visitMemberAssignmentNode(ast::MemberAssignmentNode* node) override;
        value::Value visitMethodCallNode(ast::MethodCallNode* node) override;
        value::Value visitSuperConstructorCallNode(ast::SuperConstructorCallNode* node) override;
        value::Value visitSuperMethodCallNode(ast::SuperMethodCallNode* node) override;

        // Interfaces
        value::Value visitInterfaceNode(ast::InterfaceNode* node) override;

        // Arrays
        value::Value visitArrayCreationNode(ast::ArrayCreationNode* node) override;
        value::Value visitArrayLiteralNode(ast::ArrayLiteralNode* node) override;
        value::Value visitIndexAccessNode(ast::IndexAccessNode* node) override;
        value::Value visitIndexAssignmentNode(ast::IndexAssignmentNode* node) override;

        // Lambdas
        value::Value visitLambdaNode(ast::LambdaNode* node) override;

        // Type operations
        value::Value visitCastExpression(ast::CastExpression* node) override;
        value::Value visitInstanceOfExpression(ast::InstanceOfExpression* node) override;

        // Async operations (Phase 1 stub)
        value::Value visitAwaitExpression(ast::AwaitExpression* node) override;

        // Import
        value::Value visitImportNode(ast::ImportNode* node) override;

    private:
        // Core components
        bytecode::BytecodeProgram program;
        std::shared_ptr<environment::Environment> environment;

        // Helper components (owned by BytecodeCompiler)
        emission::BytecodeEmitter emitter;
        variables::VariableTracker variableTracker;
        variables::GlobalVariableRegistry globalRegistry;
        variables::FunctionFrameManager functionFrameManager;
        control::LoopContextManager loopManager;
        control::SwitchContextManager switchManager;
        types::GenericTypeResolver genericResolver;
        types::TypeInferenceEngine typeInference;
        types::TypeValidator typeValidator;
        registration::InterfaceRegistrar interfaceRegistrar;
        registration::ClassRegistrar classRegistrar;

        // Shared context for visitor compilers
        visitors::CompilerContext context;

        // Visitor compilers (owned by BytecodeCompiler)
        visitors::LiteralCompiler literalCompiler;
        visitors::ArrayCompiler arrayCompiler;
        visitors::ExpressionCompiler expressionCompiler;
        visitors::StatementCompiler statementCompiler;
        visitors::ControlFlowCompiler controlFlowCompiler;
        visitors::FunctionCompiler functionCompiler;
        visitors::ClassCompiler classCompiler;

        // Import tracking (to avoid recompiling the same file)
        std::unordered_set<std::string> compiledImports;

        // Helper methods for coordination
        void registerClassesForBytecode(ast::ASTNode* node);
        void linkParentClasses(ast::ASTNode* node);
    };
}

