#include "BytecodeCompiler.hpp"
#include <cstddef>
#include "../../ast/nodes/expressions/AwaitExpression.hpp"
#include "../../ast/nodes/annotations/AnnotationDeclarationNode.hpp"
#include "../../runtimeTypes/klass/AnnotationDefinition.hpp"
#include "../../runtimeTypes/klass/AnnotationParamSchema.hpp"
#include "../../environment/registry/AnnotationRegistry.hpp"
#include <fstream>
#include "../../ast/nodes/statements/ImportNode.hpp"
#include "../../services/ImportManager.hpp"
#include "../../environment/registry/ExportRegistry.hpp"
#include "../../errors/TypeException.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include "../../types/TypeConversionUtils.hpp"
#include "../../types/TypeConversionBridge.hpp"
#include "analysis/NestedReferenceCollector.hpp"
#include <stdexcept>

namespace
{
    using FunctionMetadata = vm::bytecode::BytecodeProgram::FunctionMetadata;

    void applyBuiltinNativeMetadata(const std::string& name, FunctionMetadata& metadata)
    {
        if (name == "delay")
        {
            metadata.parameterCount = 1;
            metadata.parameterNames = {"ms"};
            metadata.parameterTypes = {"int"};
            metadata.parameterNullable = {false};
            metadata.returnType = "Promise<void>";
        }
        else if (name == "delayReject")
        {
            metadata.parameterCount = 2;
            metadata.parameterNames = {"ms", "exception"};
            metadata.parameterTypes = {"int", "Exception"};
            metadata.parameterNullable = {false, false};
            metadata.returnType = "Promise<void>";
        }
        else if (name == "__random_nextInt")
        {
            metadata.parameterCount = 2;
            metadata.parameterNames = {"min", "max"};
            metadata.parameterTypes = {"int", "int"};
            metadata.parameterNullable = {false, false};
            metadata.returnType = "int";
        }
        else if (name == "__random_nextFloat")
        {
            metadata.parameterCount = 2;
            metadata.parameterNames = {"min", "max"};
            metadata.parameterTypes = {"float", "float"};
            metadata.parameterNullable = {false, false};
            metadata.returnType = "float";
        }
    }

}
namespace vm::compiler
{
    BytecodeCompiler::BytecodeCompiler(std::shared_ptr<environment::Environment> env,
                                       bool skipStrictValidation)
        : environment(env)
        , emitter(program)
        , typeInference(program, env, variableTracker, globalRegistry)
        , typeValidator(env)
        , interfaceRegistrar(env, typeSubstitutionService)
        , classRegistrar(env, program, &interfaceRegistrar)
        , functionRegistrar(env, program)
        , compileTimeValidator(std::make_unique<validation::CompileTimeValidator>(env, program))
        , fieldInitValidator(std::make_unique<validation::FieldInitializationValidator>(env))
        , staticFieldInitDetector(std::make_shared<circularDependency::CircularDependencyDetector>())
        , context(*this, program, env, emitter, variableTracker, globalRegistry,
                  functionFrameManager, loopManager, switchManager, exceptionManager,
                  typeInference, typeValidator, typeSubstitutionService)
        , literalCompiler(context)
        , arrayCompiler(context)
        , expressionCompiler(context, literalCompiler, arrayCompiler)
        , statementCompiler(context)
        , controlFlowCompiler(context)
        , functionCompiler(context)
        , classCompiler(context)
        , skipStrictValidation(skipStrictValidation)
    {
        // Set up type inference engine to use context's generic type bindings stack
        typeInference.setGenericTypeBindingsStack(&context.genericTypeBindingStack);

        // PHASE 3: Set up type inference engine to use context's resolved function call types cache
        typeInference.setResolvedFunctionCallTypes(&context.resolvedFunctionCallTypes);

        // Wire null narrowing tracker for nullable expression inference
        typeInference.setNullNarrowingTracker(&context.nullNarrowing);

        // Set up compile-time validator in context and registrar
        context.compileTimeValidator = compileTimeValidator.get();
        classRegistrar.setCompileTimeValidator(compileTimeValidator.get());

        // MYT-35 follow-up — give the registrar a back-pointer so analyzer
        // checks (MYT-50 missing-@Override, etc.) can push warnings into
        // this compiler's sink. Picked up by the CLI driver after compile().
        classRegistrar.setBytecodeCompiler(this);

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
            applyBuiltinNativeMetadata(name, metadata);
            program.registerFunction(name, metadata);
        }

        // Second, pre-register all function signatures (allows forward references and mutual recursion)
        functionRegistrar.registerFunctionSignatures(root);

        // MYT-108: pre-register annotation type declarations so usage validation
        // (which runs inside class registration) can resolve user-defined annotations.
        preRegisterAnnotationDeclarations(root);

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
        context.variableTracker.beginScope();

        // MYT-XXX (top-level decl promotion): build the set of identifier
        // names referenced from any nested non-lambda function/method body
        // BEFORE visiting top-level statements, so emitVariableDeclaration
        // can decide global-vs-local at the point of emission. Lambdas are
        // excluded — captureScopeVariables handles them via the existing
        // capture path. Imports stay globals (handled by inImportedFile).
        {
            auto refs = analysis::NestedReferenceCollector::collect(root);
            context.nestedReferencesPessimistic = refs.pessimistic;
            context.namesReferencedByNestedNonLambdaFns = std::move(refs.names);
        }

        // Visit the root node to generate bytecode
        root->accept(*this);

        // Publish the top-level local count before tearing the frame down
        // — OSR needs this to tier-up loops at script scope (the top-level
        // isn't registered as a FunctionMetadata).
        const size_t topLevelLocalCount = context.functionFrameManager.getLocalCount();
        program.setTopLevelLocalCount(topLevelLocalCount);

        std::vector<std::string> topLevelLocalNames = program.getTopLevelLocalNames();
        if (topLevelLocalNames.size() < topLevelLocalCount) {
            topLevelLocalNames.resize(topLevelLocalCount);
        }
        for (const auto& local : context.variableTracker.getLocals()) {
            if (local.slot < topLevelLocalNames.size()) {
                topLevelLocalNames[local.slot] = local.name;
            }
        }
        program.setTopLevelLocalNames(topLevelLocalNames);

        // Exit the implicit main function frame
        context.variableTracker.endScope();
        context.variableTracker.endScope();
        context.functionFrameManager.exitFunctionFrame();

        // Validate all class methods have bytecode implementations
        // Skip validation in Release mode as AST optimizer may have removed unused methods
        if (!skipStrictValidation) {
            classRegistrar.validateAllClassesHaveBytecode(root);
        }

        // Emit halt instruction
        context.emitter.emitWithLocation(bytecode::OpCode::HALT, root);

        // Bytecode-level post-processing — runs the LoopOptimization,
        // Peephole, TrivialSetterInlining, TrivialGetterInlining, and
        // LocalArrayFusion passes in registration order. See
        // mType/vm/optimization/ for the pipeline.
        bytecodeOptimizationService.optimize(program);

        // MYT-318: validate operand-count contract before execution so the
        // hot-path executors covered by the validator's table can drop their
        // runtime defensive checks. Runs after every transformation pass.
        program.validateInstructionOperands();

        return std::move(program);
    }

    void BytecodeCompiler::preRegisterAnnotationDeclarations(ast::ASTNode* node)
    {
        if (!node) return;

        // If this node is itself an annotation declaration, register it.
        if (auto* declNode = dynamic_cast<ast::AnnotationDeclarationNode*>(node))
        {
            visitAnnotationDeclarationNode(declNode);
            return;
        }

        // Otherwise recurse through container nodes that may hold top-level
        // declarations (the annotation keyword is only legal at top level).
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                preRegisterAnnotationDeclarations(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                preRegisterAnnotationDeclarations(statement.get());
            }
        }
        else if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                preRegisterAnnotationDeclarations(importNode->getImportedAST());
            }
        }
    }

    void BytecodeCompiler::registerClassesForBytecode(ast::ASTNode* node)
    {
        // Register interfaces FIRST, then classes can validate against them
        interfaceRegistrar.registerInterfaces(node);

        // Apply aliases after interfaces registered (so "implements Cmp" works)
        applyImportAliases(node);

        // Register classes (may reference aliased interfaces)
        classRegistrar.registerClasses(node);

        // Apply aliases again after classes registered (so "MyInt" class alias works)
        applyImportAliases(node);

        // Extract interface metadata for bytecode serialization
        extractInterfaceMetadata(node);
    }

    void BytecodeCompiler::extractInterfaceMetadata(ast::ASTNode* node)
    {
        if (!node) return;

        if (auto interfaceNode = dynamic_cast<ast::nodes::classes::InterfaceNode*>(node))
        {
            bytecode::BytecodeProgram::InterfaceMetadata meta;
            meta.name = interfaceNode->getName();
            meta.isFinal = interfaceNode->isFinal();

            // Extract generic parameters
            const auto& genericParams = interfaceNode->getGenericParameters();
            for (const auto& param : genericParams) {
                meta.genericParameters.push_back(param.name);
            }

            // Extract extended interfaces
            meta.extendsInterfaces = interfaceNode->getExtendedInterfaces();

            // Extract method signatures
            for (const auto& method : interfaceNode->getMethods()) {
                if (auto* funcNode = dynamic_cast<ast::FunctionNode*>(method.get())) {
                    bytecode::BytecodeProgram::InterfaceMethodSignature sig;
                    sig.name = funcNode->getName();
                    sig.returnType = ::types::TypeConversionUtils::getTypeDisplayName(funcNode->getReturnType());

                    // Extract parameter types and names
                    const auto& params = funcNode->getGenericParameters();
                    for (const auto& [paramName, genType] : params) {
                        value::ValueType vType = value::ValueType::VOID;
                        if (genType) {
                            auto uType = ::types::TypeConversionBridge::toUnifiedType(genType);
                            vType = uType->isGenericParameter() ? value::ValueType::OBJECT : uType->toValueType();
                        }
                        sig.parameterTypes.push_back(::types::TypeConversionUtils::getTypeDisplayName(vType));
                        sig.parameterNames.push_back(paramName);
                    }

                    // Extract generic type parameters of the method itself
                    const auto& methodGenericParams = funcNode->getGenericTypeParameters();
                    for (const auto& gp : methodGenericParams) {
                        sig.genericTypeParameters.push_back(gp.name);
                    }

                    meta.methods.push_back(sig);
                }
            }

            program.addInterface(meta);
            return;
        }

        // Recurse into child nodes
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements()) {
                extractInterfaceMetadata(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements()) {
                extractInterfaceMetadata(statement.get());
            }
        }
        else if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            if (importNode->isResolved() && importNode->getImportedAST()) {
                extractInterfaceMetadata(importNode->getImportedAST());
            }
        }
    }

    void BytecodeCompiler::linkParentClasses(ast::ASTNode* node)
    {
        // Delegate to ClassRegistrar
        classRegistrar.linkParentClasses(node);
    }

    void BytecodeCompiler::applyImportAliases(ast::ASTNode* node)
    {
        if (!node) return;

        if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            if ((importNode->isSelective() || importNode->isLibrarySelective())
                && !importNode->getSymbolAliases().empty())
            {
                auto classRegistry = environment->getClassRegistry();
                auto ifaceRegistry = environment->getInterfaceRegistry();
                auto funcRegistry = environment->getFunctionRegistry();

                for (const auto& [original, alias] : importNode->getSymbolAliases()) {
                    // Register alias as additional name (keep original for bytecode lookup)
                    if (classRegistry) {
                        auto classDef = classRegistry->findClass(original);
                        if (classDef) {
                            classRegistry->registerClass(alias, classDef);
                        }
                    }
                    if (ifaceRegistry && ifaceRegistry->hasInterface(original)) {
                        auto ifaceDef = ifaceRegistry->findInterface(original);
                        if (ifaceDef) {
                            ifaceRegistry->registerInterface(alias, ifaceDef);
                        }
                    }
                    if (funcRegistry && funcRegistry->hasFunction(original)) {
                        auto funcDef = funcRegistry->findFunction(original);
                        if (funcDef) {
                            funcRegistry->registerFunction(alias, funcDef);
                        }
                    }
                }
            }

            // Recurse into imported AST
            if (importNode->isResolved() && importNode->getImportedAST()) {
                applyImportAliases(importNode->getImportedAST());
            }
            return;
        }

        // Recurse into child nodes
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node)) {
            for (const auto& stmt : programNode->getStatements()) {
                applyImportAliases(stmt.get());
            }
        } else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node)) {
            for (const auto& stmt : blockNode->getStatements()) {
                applyImportAliases(stmt.get());
            }
        }
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

    value::Value BytecodeCompiler::visitMatchNode(ast::MatchNode* node)
    {
        return controlFlowCompiler.compileMatch(node);
    }

    value::Value BytecodeCompiler::visitMatchCaseNode(ast::MatchCaseNode* node)
    {
        // Handled inline by compileMatch
        return std::monostate{};
    }

    value::Value BytecodeCompiler::visitMatchDefaultNode(ast::MatchDefaultNode* node)
    {
        // Handled inline by compileMatch
        return std::monostate{};
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
        // Library imports are resolved by LibraryLinker at project build time, not here
        if (node->isLibraryImport()) {
            return std::monostate{};
        }

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
                                "Symbol not found",
                                node->getLocation()
                            );
                        }

                        // Check if symbol is public (exported)
                        if (!exportRegistry->isSymbolExported(resolvedPath, symbolName)) {
                            throw errors::TypeException(
                                "Cannot import '" + symbolName + "' from '" + filePath + "': " +
                                "Symbol is private and not exported. Only public symbols can be imported.",
                                node->getLocation()
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
            // This will register all functions/methods in the BytecodeProgram.
            // MYT-XXX: while compiling an imported file, top-level decls must
            // stay globals (other modules may import their PUBLIC names);
            // gate the promotion path with this flag.
            {
                visitors::ImportedFileContextGuard guard(context, true);
                importedAST->accept(*this);
            }

            // Register bytecode function aliases for constructors/methods
            // Must happen AFTER compilation (importedAST->accept) so constructors exist
            if ((node->isSelective() || node->isLibrarySelective())
                && !node->getSymbolAliases().empty())
            {
                for (const auto& [original, alias] : node->getSymbolAliases()) {
                    std::string origPrefix = original + "::";
                    std::string aliasPrefix = alias + "::";
                    // Collect names first to avoid modifying map during iteration
                    std::vector<std::pair<std::string, bytecode::BytecodeProgram::FunctionMetadata>> toAdd;
                    for (const auto& [funcName, funcMeta] : program.getFunctions()) {
                        if (funcName.size() > origPrefix.size() &&
                            funcName.substr(0, origPrefix.size()) == origPrefix) {
                            std::string aliasedName = aliasPrefix + funcName.substr(origPrefix.size());
                            if (!program.getFunction(aliasedName)) {
                                toAdd.emplace_back(aliasedName, funcMeta);
                            }
                        }
                    }
                    for (auto& [name, meta] : toAdd) {
                        program.registerFunction(name, meta);
                    }
                }
            }

            // Note: Global variables from imported files are registered in globalRegistry during compilation
            // and persist across file boundaries (see GlobalVariableRegistry::removeVariablesOutOfScope)

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

    value::Value BytecodeCompiler::visitAnnotationDeclarationNode(ast::AnnotationDeclarationNode* node)
    {
        using ast::nodes::annotations::AnnotationValueType;

        auto annotationRegistry = environment->getAnnotationRegistry();
        if (!annotationRegistry) return value::Value();

        // Don't shadow built-ins (Override/Script/EntryPoint/Throw) — they're
        // pre-registered at Environment::initialize() and re-defining them is
        // caught earlier by the duplicate-type-name check in the parser.
        if (annotationRegistry->hasAnnotation(node->getName())) return value::Value();

        // Register at runtime for the validation/reflection passes.
        auto def = std::make_shared<runtimeTypes::klass::AnnotationDefinition>(node->getName(), false);
        for (const auto& declParam : node->getParams())
        {
            runtimeTypes::klass::AnnotationParamSchema schema;
            schema.name          = declParam.name;
            schema.declaredType  = declParam.declaredType;
            schema.defaultValue  = declParam.defaultValue;
            schema.nullable      = declParam.nullable;
            schema.isArray       = declParam.isArray;
            def->addParam(std::move(schema));
        }
        for (const auto& meta : node->getMetaAnnotations())
        {
            if (meta) def->addMetaAnnotation(meta);
        }
        annotationRegistry->registerAnnotation(node->getName(), def);

        // Also serialize into the bytecode program (.mtc v5+ section) so the
        // declaration survives compile-to-file round-trips.
        bytecode::BytecodeProgram::AnnotationDeclData declData;
        declData.name = node->getName();
        for (const auto& declParam : node->getParams())
        {
            bytecode::BytecodeProgram::AnnotationParamSchemaData p;
            p.name         = declParam.name;
            p.declaredType = static_cast<uint8_t>(declParam.declaredType);
            p.nullable     = declParam.nullable;
            p.isArray      = declParam.isArray;
            p.hasDefault   = declParam.defaultValue.has_value();
            if (p.hasDefault)
            {
                const auto& dv = *declParam.defaultValue;
                switch (dv.getType())
                {
                case AnnotationValueType::INT:         p.defaultInt = dv.asInt(); break;
                case AnnotationValueType::FLOAT:       p.defaultFloat = dv.asFloat(); break;
                case AnnotationValueType::BOOL:        p.defaultBool = dv.asBool(); break;
                case AnnotationValueType::STRING:      p.defaultString = dv.asString(); break;
                case AnnotationValueType::CLASS_REF:   p.defaultString = dv.asClassRef(); break;
                case AnnotationValueType::CLASS_ARRAY: p.defaultStringArray = dv.asClassArray(); break;
                case AnnotationValueType::NULL_VALUE:  break; // no payload needed
                }
            }
            declData.params.push_back(std::move(p));
        }
        // MYT-109 (.mtc v6+): serialize meta-annotations applied to this
        // annotation declaration so `@Retention` / `@Target` / user-defined
        // meta-annotations survive a compile-to-file round-trip.
        for (const auto& meta : node->getMetaAnnotations())
        {
            if (!meta) continue;
            bytecode::BytecodeProgram::AnnotationData annot;
            annot.name = meta->getName();
            annot.location = meta->getLocation();
            for (const auto& key : meta->getKeyOrder())
            {
                if (const auto* v = meta->getTypedParameter(key))
                {
                    bytecode::BytecodeProgram::TypedAnnotationArg arg;
                    arg.key = key;
                    arg.valueType = static_cast<uint8_t>(v->getType());
                    switch (v->getType())
                    {
                    case AnnotationValueType::INT:         arg.intVal    = v->asInt(); break;
                    case AnnotationValueType::FLOAT:       arg.floatVal  = v->asFloat(); break;
                    case AnnotationValueType::BOOL:        arg.boolVal   = v->asBool(); break;
                    case AnnotationValueType::STRING:      arg.stringVal = v->asString(); break;
                    case AnnotationValueType::CLASS_REF:   arg.stringVal = v->asClassRef(); break;
                    case AnnotationValueType::CLASS_ARRAY: arg.arrayVal  = v->asClassArray(); break;
                    case AnnotationValueType::NULL_VALUE:  break;
                    }
                    annot.typedArguments.push_back(std::move(arg));
                }
            }
            declData.metaAnnotations.push_back(std::move(annot));
        }
        program.addAnnotationDeclaration(std::move(declData));
        return value::Value();
    }

} // namespace vm::compiler
