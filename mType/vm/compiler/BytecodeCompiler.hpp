#pragma once
#include "../../ast/ASTVisitor.hpp"
#include "../../ast/NodeClassesDeclaration.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../../environment/Environment.hpp"
#include "../../token/TokenType.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace vm::compiler
{
    /**
     * Compiles AST nodes to bytecode instructions
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

        // Import
        value::Value visitImportNode(ast::ImportNode* node) override;

    private:
        bytecode::BytecodeProgram program;
        std::shared_ptr<environment::Environment> environment;

        // Compilation state
        struct LoopContext {
            std::vector<size_t> breakJumps;    // Jump instructions that need to be patched to loop end
            std::vector<size_t> continueJumps; // Jump instructions that need to be patched to continue target
            size_t loopStart;                   // Offset of loop start (condition check)
            size_t continueTarget;              // Offset where continue should jump (for loops: increment, others: loopStart)
        };
        std::vector<LoopContext> loopStack;

        struct SwitchContext {
            std::vector<size_t> breakJumps;    // Jump instructions that need to be patched to switch end
        };
        std::vector<SwitchContext> switchStack;

        // Local variable tracking (for optimization)
        struct LocalVariable {
            std::string name;
            size_t slot;
            int scopeDepth;
            value::ValueType type = value::ValueType::VOID;
            std::string className;  // For OBJECT types (interfaces/classes)
        };
        std::vector<LocalVariable> locals;
        size_t nextLocalSlot = 0;
        int currentScopeDepth = 0;

        // Global variable tracking (for compile-time validation)
        std::unordered_set<std::string> globalVariables;
        std::unordered_map<std::string, value::ValueType> globalVariableTypes;
        std::unordered_map<std::string, std::string> globalVariableClassNames;  // For OBJECT types
        std::unordered_map<std::string, int> globalVariableScopes;  // Track scope depth where global var was declared

        // Import tracking (to avoid recompiling the same file)
        std::unordered_set<std::string> compiledImports;

        // Function frame tracking
        struct FunctionFrame {
            size_t localStartSlot;
            int scopeDepthStart;
            std::string returnType;
            bool isLambda = false;  // Track if this frame is for a lambda
            size_t maxLocalSlot = 0;  // Track the maximum local slot used in this function
        };
        std::vector<FunctionFrame> functionFrameStack;

        // Closure tracking
        struct ClosureVariable {
            std::string name;
            size_t slot;
            bool isFromParent;
        };
        std::vector<ClosureVariable> closureCaptures;

        // Class/Method context tracking
        ast::ClassNode* currentClassNode = nullptr;
        bool inInstanceMethod = false;

        // Helper methods
        void emitWithLocation(bytecode::OpCode opcode, ast::ASTNode* node);
        void emitWithLocation(bytecode::OpCode opcode, uint32_t operand, ast::ASTNode* node);
        size_t emitJump(bytecode::OpCode jumpOp);
        void patchJump(size_t offset);
        void emitLoop(size_t loopStart);

        // Class registration for bytecode
        void registerClassesForBytecode(ast::ASTNode* node);
        void registerInterfaceForBytecode(ast::nodes::classes::InterfaceNode* interfaceNode);
        void linkParentClasses(ast::ASTNode* node);
        bytecode::BytecodeProgram::ClassMetadata extractClassMetadata(ast::ClassNode* classNode);

        // Interface validation
        void validateInterfaceImplementations(
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
            const ast::SourceLocation& location);
        std::pair<std::string, std::vector<std::string>> parseGenericInterfaceName(
            const std::string& interfaceName);
        std::string resolveGenericType(
            const std::string& typeName,
            const std::unordered_map<std::string, std::string>& substitutions);

        // Type conversion helpers
        bytecode::OpCode getBinaryOpCode(token::TokenType op, bool typeSpecialized = false);
        bytecode::OpCode getUnaryOpCode(token::TokenType op);
        value::ValueType inferExpressionType(ast::ASTNode* node);
        std::string inferExpressionClassName(ast::ASTNode* node);
        bool isClassCompatible(const std::string& derivedClass, const std::string& baseClass);

        // Variable management
        size_t resolveLocal(const std::string& name);
        void beginScope();
        void endScope();

        // Function frame management
        void enterFunctionFrame(const std::string& returnType, bool isLambda = false);
        void exitFunctionFrame();
        size_t getLocalCount() const;

        // Loop management
        void enterLoop(size_t loopStart, size_t continueTarget = SIZE_MAX);
        void exitLoop();
        LoopContext& currentLoop();
    };
}

