#include "BytecodeCompiler.hpp"
#include "../../ast/nodes/expressions/AwaitExpression.hpp"
#include <unordered_set>
#include "../../ast/nodes/statements/ImportNode.hpp"
#include "../../services/ImportManager.hpp"
#include "../../environment/registry/ExportRegistry.hpp"
#include "../../errors/TypeException.hpp"
#include "../runtime/optimization/LoopOptimizer.hpp"
#include "../optimization/PeepholeOptimizer.hpp"
#include <stdexcept>
#include <iostream>
namespace vm::compiler
{
    BytecodeCompiler::BytecodeCompiler(std::shared_ptr<environment::Environment> env,
                                       bool skipStrictValidation,
                                       constants::OptimizationLevel optimizationLevel)
        : environment(env)
        , emitter(program)
        , typeInference(program, env, variableTracker, globalRegistry)
        , typeValidator(env)
        , interfaceRegistrar(env, genericResolver)
        , classRegistrar(env, program, &interfaceRegistrar)
        , functionRegistrar(env, program)
        , compileTimeValidator(std::make_unique<validation::CompileTimeValidator>(env, program))
        , fieldInitValidator(std::make_unique<validation::FieldInitializationValidator>(env))
        , staticFieldInitDetector(std::make_shared<circularDependency::CircularDependencyDetector>())
        , context(*this, program, env, emitter, variableTracker, globalRegistry,
                  functionFrameManager, loopManager, switchManager, exceptionManager,
                  typeInference, typeValidator, genericResolver)
        , literalCompiler(context)
        , arrayCompiler(context)
        , expressionCompiler(context, literalCompiler, arrayCompiler)
        , statementCompiler(context)
        , controlFlowCompiler(context)
        , functionCompiler(context)
        , classCompiler(context)
        , skipStrictValidation(skipStrictValidation)
        , optimizationLevel(optimizationLevel)
    {
        // Set up type inference engine to use context's generic type bindings stack
        typeInference.setGenericTypeBindingsStack(&context.genericTypeBindingStack);

        // PHASE 3: Set up type inference engine to use context's resolved function call types cache
        typeInference.setResolvedFunctionCallTypes(&context.resolvedFunctionCallTypes);

        // Set up compile-time validator in context and registrar
        context.compileTimeValidator = compileTimeValidator.get();
        classRegistrar.setCompileTimeValidator(compileTimeValidator.get());

        // Set up static field initialization detector in context
        context.staticFieldInitDetector = staticFieldInitDetector;
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

        // Second, pre-register all function signatures (allows forward references and mutual recursion)
        functionRegistrar.registerFunctionSignatures(root);

        // Third, register all classes and interfaces using registrars
        registerClassesForBytecode(root);

        // Fourth, establish parent-child relationships
        linkParentClasses(root);

        // Fifth, validate field initialization dependencies (detect circular references)
        fieldInitValidator->validateFieldInitializations(root);

        // Sixth, validate @Throw annotations now that all classes are registered
        functionRegistrar.validateThrowAnnotations(root);

        // Create implicit "main" function frame for global scope
        // This allows variables at global scope to be tracked and captured by lambdas
        context.functionFrameManager.enterFunctionFrame("", // Empty name = global scope
                                                        "void",
                                                        context.variableTracker.getNextLocalSlot(),
                                                        context.variableTracker.getCurrentScopeDepth(),
                                                        false, // Not a lambda
                                                        false); // Not async
        context.variableTracker.beginScope();

        // Visit the root node to generate bytecode
        root->accept(*this);

        // Exit the implicit main function frame
        context.variableTracker.endScope();
        context.functionFrameManager.exitFunctionFrame();

        // Validate all class methods have bytecode implementations
        // Skip validation in Release mode as AST optimizer may have removed unused methods
        if (!skipStrictValidation) {
            classRegistrar.validateAllClassesHaveBytecode(root);
        }

        // Emit halt instruction
        context.emitter.emitWithLocation(bytecode::OpCode::HALT, root);

        // OPTIMIZATION PASS: Run loop optimizer after bytecode generation
        // This analyzes LOOP_START/LOOP_END markers and applies optimizations
        runtime::optimization::LoopOptimizer loopOptimizer(program);
        loopOptimizer.optimize();

        // PEEPHOLE OPTIMIZATION PASS: Only run in Release mode
        if (optimizationLevel == constants::OptimizationLevel::Release) {
            auto config = optimization::PeepholeOptimizer::Config::forReleaseMode();

            optimization::PeepholeOptimizer peepholeOptimizer(config);
            peepholeOptimizer.registerDefaultPatterns();

            try {
                peepholeOptimizer.optimize(program);
            } catch (const std::exception& e) {
                std::cerr << "WARNING: Peephole optimization failed: " << e.what() << std::endl;
                std::cerr << "Continuing with unoptimized bytecode..." << std::endl;
            }
        }

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

    value::Value BytecodeCompiler::visitThisConstructorCallNode(ast::ThisConstructorCallNode* node)
    {
        return classCompiler.compileThisConstructorCall(node);
    }

    value::Value BytecodeCompiler::visitSuperMethodCallNode(ast::SuperMethodCallNode* node)
    {
        return classCompiler.compileSuperMethodCall(node);
    }

    value::Value BytecodeCompiler::visitSuperMemberAccessNode(ast::SuperMemberAccessNode* node)
    {
        return classCompiler.compileSuperMemberAccess(node);
    }

    value::Value BytecodeCompiler::visitSuperMemberAssignmentNode(ast::SuperMemberAssignmentNode* node)
    {
        return classCompiler.compileSuperMemberAssignment(node);
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

    value::Value BytecodeCompiler::visitAwaitExpression(ast::AwaitExpression* node)
    {
        // Compile the expression being awaited (should evaluate to a Promise)
        node->getExpressionPtr()->accept(*this);

        // Emit AWAIT instruction to unwrap the Promise
        // In Phase 2 synchronous model, this immediately returns the Promise's value
        context.emitter.emitWithLocation(bytecode::OpCode::AWAIT, node);

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitImportNode(ast::ImportNode* node)
    {
        // Handle imports at compile time
        std::string filePath = node->getFilePath();

        // Get ImportManager from environment
        auto importManager = environment->getImportManager();
        if (!importManager) {
            throw std::runtime_error("Import manager not available for compilation");
        }

        // Save current file path for proper restoration
        std::string savedCurrentFile = importManager->getCurrentFilePath();

        // Resolve the import path (relative to current file)
        std::string resolvedPath = importManager->resolvePath(filePath);

        // Check if already compiled to avoid re-compilation
        if (compiledImports.find(resolvedPath) != compiledImports.end()) {
            return std::monostate{};
        }

        // Mark as being compiled
        compiledImports.insert(resolvedPath);

        try {
            // Parse and get the AST for the imported file BEFORE setting current file
            // This ensures parseAndCacheAST uses the correct context
            auto* importedAST = importManager->parseAndCacheAST(filePath);
            if (!importedAST) {
                throw std::runtime_error("Failed to parse import: " + filePath);
            }

            // Set current file to the resolved path AFTER parsing
            // This ensures that if the imported file has its own imports,
            // they will be resolved relative to the imported file's directory
            importManager->setCurrentFilePath(resolvedPath);

            // IMPORTANT: Process nested imports FIRST
            // This ensures all dependencies are loaded before class registration
            importHelper.processNestedImports(importedAST, [this](ast::ImportNode* node) {
                visitImportNode(node);
            });

            // Validate selective imports - check that imported symbols are public
            if (node->isSelective()) {
                auto exportRegistry = environment->getExportRegistry();
                if (exportRegistry) {
                    // Collect exported symbols from the imported file
                    importHelper.collectExportedSymbols(importedAST, resolvedPath, exportRegistry);

                    // Validate each requested symbol
                    const auto& requestedSymbols = node->getImportedSymbols();
                    for (const auto& symbolName : requestedSymbols) {
                        // Check if symbol exists
                        if (!exportRegistry->symbolExists(resolvedPath, symbolName)) {
                            throw errors::TypeException(
                                "Cannot import '" + symbolName + "' from '" + filePath + "': " +
                                "Symbol not found"
                            );
                        }

                        // Check if symbol is public (exported)
                        if (!exportRegistry->isSymbolExported(resolvedPath, symbolName)) {
                            throw errors::TypeException(
                                "Cannot import '" + symbolName + "' from '" + filePath + "': " +
                                "Symbol is private and not exported. Only public symbols can be imported."
                            );
                        }
                    }
                }
            }

            // IMPORTANT: Register classes and interfaces AFTER nested imports are processed
            // This ensures that parent classes from nested imports are available
            registerClassesForBytecode(importedAST);
            linkParentClasses(importedAST);

            // Validate @Throw annotations now that all classes are registered
            functionRegistrar.validateThrowAnnotations(importedAST);

            // Compile the imported file to generate bytecode for functions and methods
            // This will register all functions/methods in the BytecodeProgram
            importedAST->accept(*this);

            // Restore previous current file
            importManager->setCurrentFilePath(savedCurrentFile);
        }
        catch (...) {
            // Restore current file on error
            importManager->setCurrentFilePath(savedCurrentFile);
            throw;
        }

        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitTryNode(ast::TryNode* node)
    {
        return controlFlowCompiler.compileTry(node);
    }

    value::Value BytecodeCompiler::visitCatchNode(ast::CatchNode* node)
    {
        // Catch nodes should not be visited directly
        throw std::runtime_error("Catch nodes should only be processed within try statements");
    }

    value::Value BytecodeCompiler::visitThrowNode(ast::ThrowNode* node)
    {
        return controlFlowCompiler.compileThrow(node);
    }

    value::Value BytecodeCompiler::visitAnnotationNode(ast::AnnotationNode* node)
    {
        // Annotations are metadata only - no bytecode generation needed
        // They are processed during semantic analysis phase, not compilation
        return value::Value();
    }

} // namespace vm::compiler
