#include "BytecodeCompiler.hpp"
#include "../../evaluator/utils/ValueConverter.hpp"
#include <unordered_set>
#include "../../ast/nodes/statements/ImportNode.hpp"
#include <stdexcept>

namespace vm::compiler
{
    BytecodeCompiler::BytecodeCompiler(std::shared_ptr<environment::Environment> env)
        : environment(env)
        , emitter(program)
        , typeInference(program, env, variableTracker, globalRegistry)
        , typeValidator(env)
        , interfaceRegistrar(env, genericResolver)
        , classRegistrar(env, program, &interfaceRegistrar)
        , context(*this, program, env, emitter, variableTracker, globalRegistry,
                  functionFrameManager, loopManager, switchManager,
                  typeInference, typeValidator, genericResolver)
        , literalCompiler(context)
        , arrayCompiler(context)
        , expressionCompiler(context, literalCompiler, arrayCompiler)
        , statementCompiler(context)
        , controlFlowCompiler(context)
        , functionCompiler(context)
        , classCompiler(context)
    {
        // Set up type inference engine to use context's generic type bindings stack
        typeInference.setGenericTypeBindingsStack(&context.genericTypeBindingStack);
    }

    bytecode::BytecodeProgram BytecodeCompiler::compile(ast::ASTNode* root)
    {
        if (!root) {
            throw std::runtime_error("Cannot compile null AST root");
        }

        // First, register all native functions from the environment
        auto nativeFunctionNames = environment->getNativeRegistry()->getAllNativeFunctionNames();
        for (const auto& name : nativeFunctionNames) {
            bytecode::BytecodeProgram::FunctionMetadata metadata;
            metadata.name = name;
            metadata.isNative = true;
            metadata.parameterCount = 0;
            metadata.startOffset = 0;
            metadata.returnType = "";
            program.registerFunction(name, metadata);
        }

        // Second, register all classes and interfaces using registrars
        registerClassesForBytecode(root);

        // Third, establish parent-child relationships
        linkParentClasses(root);

        // Visit the root node to generate bytecode
        root->accept(*this);

        // Emit halt instruction
        program.emit(bytecode::OpCode::HALT);

        return std::move(program);
    }

    void BytecodeCompiler::registerClassesForBytecode(ast::ASTNode* node)
    {
        // Register interfaces FIRST, then classes can validate against them
        interfaceRegistrar.registerInterfaces(node);
        classRegistrar.registerClasses(node);
    }

    void BytecodeCompiler::linkParentClasses(ast::ASTNode* node)
    {
        // Delegate to ClassRegistrar
        classRegistrar.linkParentClasses(node);
    }

    // ==================== AST Visitor Implementations (all delegated) ====================

    value::Value BytecodeCompiler::visitProgramNode(ast::ProgramNode* node)
    {
        return statementCompiler.compileProgram(node);
    }

    value::Value BytecodeCompiler::visitBlockNode(ast::BlockNode* node)
    {
        return statementCompiler.compileBlock(node);
    }

    value::Value BytecodeCompiler::visitIntegerNode(ast::IntegerNode* node)
    {
        return literalCompiler.compileInteger(node);
    }

    value::Value BytecodeCompiler::visitFloatNode(ast::FloatNode* node)
    {
        return literalCompiler.compileFloat(node);
    }

    value::Value BytecodeCompiler::visitStringNode(ast::StringNode* node)
    {
        return literalCompiler.compileString(node);
    }

    value::Value BytecodeCompiler::visitBoolNode(ast::BoolNode* node)
    {
        return literalCompiler.compileBool(node);
    }

    value::Value BytecodeCompiler::visitNullNode(ast::NullNode* node)
    {
        return literalCompiler.compileNull(node);
    }

    value::Value BytecodeCompiler::visitVariableNode(ast::VariableNode* node)
    {
        return expressionCompiler.compileVariable(node);
    }

    value::Value BytecodeCompiler::visitDeclarationNode(ast::DeclarationNode* node)
    {
        return statementCompiler.compileDeclaration(node);
    }

    value::Value BytecodeCompiler::visitAssignmentNode(ast::AssignmentNode* node)
    {
        return statementCompiler.compileAssignment(node);
    }

    value::Value BytecodeCompiler::visitBinaryOpNode(ast::BinaryOpNode* node)
    {
        return expressionCompiler.compileBinaryOp(node);
    }

    value::Value BytecodeCompiler::visitUnaryOpNode(ast::UnaryOpNode* node)
    {
        return expressionCompiler.compileUnaryOp(node);
    }

    value::Value BytecodeCompiler::visitTernaryOpNode(ast::TernaryOpNode* node)
    {
        return expressionCompiler.compileTernaryOp(node);
    }

    value::Value BytecodeCompiler::visitIfNode(ast::IfNode* node)
    {
        return controlFlowCompiler.compileIf(node);
    }

    value::Value BytecodeCompiler::visitWhileNode(ast::WhileNode* node)
    {
        return controlFlowCompiler.compileWhile(node);
    }

    value::Value BytecodeCompiler::visitDoWhileNode(ast::DoWhileNode* node)
    {
        return controlFlowCompiler.compileDoWhile(node);
    }

    value::Value BytecodeCompiler::visitForNode(ast::ForNode* node)
    {
        return controlFlowCompiler.compileFor(node);
    }

    value::Value BytecodeCompiler::visitForEachNode(ast::ForEachNode* node)
    {
        return controlFlowCompiler.compileForEach(node);
    }

    value::Value BytecodeCompiler::visitBreakNode(ast::BreakNode* node)
    {
        return controlFlowCompiler.compileBreak(node);
    }

    value::Value BytecodeCompiler::visitContinueNode(ast::ContinueNode* node)
    {
        return controlFlowCompiler.compileContinue(node);
    }

    value::Value BytecodeCompiler::visitSwitchNode(ast::SwitchNode* node)
    {
        return controlFlowCompiler.compileSwitch(node);
    }

    value::Value BytecodeCompiler::visitCaseNode(ast::CaseNode* node)
    {
        return controlFlowCompiler.compileCase(node);
    }

    value::Value BytecodeCompiler::visitDefaultCaseNode(ast::DefaultCaseNode* node)
    {
        return controlFlowCompiler.compileDefaultCase(node);
    }

    value::Value BytecodeCompiler::visitFunctionNode(ast::FunctionNode* node)
    {
        return functionCompiler.compileFunction(node);
    }

    value::Value BytecodeCompiler::visitFunctionCallNode(ast::FunctionCallNode* node)
    {
        return functionCompiler.compileFunctionCall(node);
    }

    value::Value BytecodeCompiler::visitReturnNode(ast::ReturnNode* node)
    {
        return functionCompiler.compileReturn(node);
    }

    value::Value BytecodeCompiler::visitNativeFunctionNode(ast::NativeFunctionNode* node)
    {
        return functionCompiler.compileNativeFunction(node);
    }

    value::Value BytecodeCompiler::visitLambdaNode(ast::LambdaNode* node)
    {
        return functionCompiler.compileLambda(node);
    }

    value::Value BytecodeCompiler::visitClassNode(ast::ClassNode* node)
    {
        return classCompiler.compileClass(node);
    }

    value::Value BytecodeCompiler::visitMethodNode(ast::MethodNode* node)
    {
        return classCompiler.compileMethod(node);
    }

    value::Value BytecodeCompiler::visitConstructorNode(ast::ConstructorNode* node)
    {
        return classCompiler.compileConstructor(node);
    }

    value::Value BytecodeCompiler::visitFieldNode(ast::FieldNode* node)
    {
        return classCompiler.compileField(node);
    }

    value::Value BytecodeCompiler::visitNewNode(ast::NewNode* node)
    {
        return classCompiler.compileNew(node);
    }

    value::Value BytecodeCompiler::visitMemberAccessNode(ast::MemberAccessNode* node)
    {
        return classCompiler.compileMemberAccess(node);
    }

    value::Value BytecodeCompiler::visitMemberAssignmentNode(ast::MemberAssignmentNode* node)
    {
        return classCompiler.compileMemberAssignment(node);
    }

    value::Value BytecodeCompiler::visitMethodCallNode(ast::MethodCallNode* node)
    {
        return classCompiler.compileMethodCall(node);
    }

    value::Value BytecodeCompiler::visitSuperConstructorCallNode(ast::SuperConstructorCallNode* node)
    {
        return classCompiler.compileSuperConstructorCall(node);
    }

    value::Value BytecodeCompiler::visitSuperMethodCallNode(ast::SuperMethodCallNode* node)
    {
        return classCompiler.compileSuperMethodCall(node);
    }

    value::Value BytecodeCompiler::visitInterfaceNode(ast::InterfaceNode* node)
    {
        // Interfaces are registered during registerClassesForBytecode phase
        // No bytecode needs to be generated for interface declarations
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitArrayCreationNode(ast::ArrayCreationNode* node)
    {
        return arrayCompiler.compileArrayCreation(node);
    }

    value::Value BytecodeCompiler::visitArrayLiteralNode(ast::ArrayLiteralNode* node)
    {
        return arrayCompiler.compileArrayLiteral(node);
    }

    value::Value BytecodeCompiler::visitIndexAccessNode(ast::IndexAccessNode* node)
    {
        return arrayCompiler.compileIndexAccess(node);
    }

    value::Value BytecodeCompiler::visitIndexAssignmentNode(ast::IndexAssignmentNode* node)
    {
        return arrayCompiler.compileIndexAssignment(node);
    }

    value::Value BytecodeCompiler::visitCastExpression(ast::CastExpression* node)
    {
        return expressionCompiler.compileCast(node);
    }

    value::Value BytecodeCompiler::visitInstanceOfExpression(ast::InstanceOfExpression* node)
    {
        return expressionCompiler.compileInstanceOf(node);
    }

    value::Value BytecodeCompiler::visitImportNode(ast::ImportNode* node)
    {
        // Handle imports at compile time
        std::string importPath = node->getFilePath();

        if (compiledImports.find(importPath) != compiledImports.end()) {
            return std::monostate{};
        }

        auto* importedAST = node->getImportedAST();
        if (!importedAST) {
            throw std::runtime_error("Import not resolved: " + importPath);
        }

        compiledImports.insert(importPath);
        importedAST->accept(*this);

        return std::monostate{};
    }

} // namespace vm::compiler
