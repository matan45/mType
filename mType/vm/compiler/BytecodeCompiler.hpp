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
#include "control/ExceptionContextManager.hpp"
#include "types/TypeInferenceEngine.hpp"
#include "types/TypeValidator.hpp"
#include "../../types/TypeSubstitutionService.hpp"
#include "registration/ClassRegistrar.hpp"
#include "registration/InterfaceRegistrar.hpp"
#include "registration/FunctionRegistrar.hpp"
#include "validation/CompileTimeValidator.hpp"
#include "validation/FieldInitializationValidator.hpp"
#include "visitors/LiteralCompiler.hpp"
#include "visitors/ArrayCompiler.hpp"
#include "visitors/ExpressionCompiler.hpp"
#include "visitors/StatementCompiler.hpp"
#include "visitors/ControlFlowCompiler.hpp"
#include "visitors/FunctionCompiler.hpp"
#include "visitors/ClassCompiler.hpp"
#include "ImportHelper.hpp"
#include "../../circularDependency/CircularDependencyDetector.hpp"
#include "../../diagnostics/Diagnostic.hpp"
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
        explicit BytecodeCompiler(std::shared_ptr<environment::Environment> env,
                                 bool skipStrictValidation = false);
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
        value::Value visitMatchNode(ast::MatchNode* node) override;
        value::Value visitMatchCaseNode(ast::MatchCaseNode* node) override;
        value::Value visitMatchDefaultNode(ast::MatchDefaultNode* node) override;

        // Functions
        value::Value visitFunctionNode(ast::FunctionNode* node) override;
        value::Value visitFunctionCallNode(ast::FunctionCallNode* node) override;
        value::Value visitReturnNode(ast::ReturnNode* node) override;

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
        value::Value visitSuperMemberAccessNode(ast::SuperMemberAccessNode* node) override;
        value::Value visitSuperMemberAssignmentNode(ast::SuperMemberAssignmentNode* node) override;
        value::Value visitThisConstructorCallNode(ast::ThisConstructorCallNode* node) override;

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

        // Exception handling
        value::Value visitTryNode(ast::TryNode* node) override;
        value::Value visitCatchNode(ast::CatchNode* node) override;
        value::Value visitThrowNode(ast::ThrowNode* node) override;

        // Annotations (metadata only - no bytecode generation)
        value::Value visitAnnotationNode(ast::AnnotationNode* node) override;

        // MYT-35 Phase-2 follow-up — non-fatal warning sink. Compile-time
        // analyzers (e.g., MYT-50 missing-@Override checker) push
        // Diagnostic objects here. The CLI driver renders them after a
        // successful compile via runMain::reportWarning().
        void addWarning(diagnostics::Diagnostic warning)
        {
            warnings_.push_back(std::move(warning));
        }
        const std::vector<diagnostics::Diagnostic>& warnings() const
        {
            return warnings_;
        }
        std::vector<diagnostics::Diagnostic> takeWarnings()
        {
            return std::move(warnings_);
        }

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
        control::ExceptionContextManager exceptionManager;
        ::types::TypeSubstitutionService typeSubstitutionService;
        types::TypeInferenceEngine typeInference;
        types::TypeValidator typeValidator;
        registration::InterfaceRegistrar interfaceRegistrar;
        registration::ClassRegistrar classRegistrar;
        registration::FunctionRegistrar functionRegistrar;
        std::unique_ptr<validation::CompileTimeValidator> compileTimeValidator;
        std::unique_ptr<validation::FieldInitializationValidator> fieldInitValidator;
        std::shared_ptr<circularDependency::CircularDependencyDetector> staticFieldInitDetector;

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

        // Import helper
        ImportHelper importHelper;

        // Import tracking (to avoid recompiling the same file)
        std::unordered_set<std::string> compiledImports;

        // Validation control
        bool skipStrictValidation;

        // MYT-35 follow-up — non-fatal warnings collected during compile.
        // Drained by the CLI driver after compile() returns; the LSP path
        // bypasses this sink and pushes directly to Document::diagnostics.
        std::vector<diagnostics::Diagnostic> warnings_;

        // Helper methods for coordination
        void registerClassesForBytecode(ast::ASTNode* node);
        void linkParentClasses(ast::ASTNode* node);
        void extractInterfaceMetadata(ast::ASTNode* node);
    };
}

