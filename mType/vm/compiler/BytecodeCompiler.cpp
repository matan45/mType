#include "BytecodeCompiler.hpp"
#include <cstddef>
#include "../../ast/nodes/annotations/AnnotationDeclarationNode.hpp"
#include "../../ast/nodes/statements/ImportNode.hpp"
#include "../../optimizer/passes/annotation_folding/AnnotationConstantResolver.hpp"
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
        typeInference.setGenericTypeBindingsStack(&context.genericTypeBindingStack);
        typeInference.setResolvedFunctionCallTypes(&context.resolvedFunctionCallTypes);
        typeInference.setNullNarrowingTracker(&context.nullNarrowing);

        context.compileTimeValidator = compileTimeValidator.get();
        classRegistrar.setCompileTimeValidator(compileTimeValidator.get());

        // MYT-35 follow-up — give the registrar a back-pointer so analyzer
        // checks (MYT-50 missing-@Override, etc.) can push warnings into
        // this compiler's sink. Picked up by the CLI driver after compile().
        classRegistrar.setBytecodeCompiler(this);

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

        // MYT-376: fold constant-expression / `Class::FIELD` annotation
        // arguments to literal TypedAnnotationValues BEFORE class registration
        // serializes annotation metadata. Imports are already resolved
        // (ScriptInterpreter::resolveImports runs before compile), so the
        // resolver also folds annotations in imported files and can resolve
        // constants declared across file boundaries.
        optimizer::passes::annotation_folding::AnnotationConstantResolver::resolve(root);

        // Third, register all classes and interfaces using registrars
        registerClassesForBytecode(root);

        // Fourth, establish parent-child relationships
        linkParentClasses(root);

        // Fifth, validate field initialization dependencies (detect circular references)
        fieldInitValidator->validateFieldInitializations(root);

        // Sixth, validate @Throw annotations now that all classes are registered
        functionRegistrar.validateThrowAnnotations(root);

        // Create implicit "main" function frame for global scope so global-scope
        // variables can be tracked and captured by lambdas.
        context.functionFrameManager.enterFunctionFrame("",
                                                        "void",
                                                        context.variableTracker.getNextLocalSlot(),
                                                        context.variableTracker.getCurrentScopeDepth(),
                                                        false,
                                                        false);
        context.variableTracker.beginScope();
        context.variableTracker.beginScope();

        // Top-level decl promotion: build the set of identifier names referenced
        // from any nested non-lambda function/method body BEFORE visiting top-level
        // statements, so emitVariableDeclaration can decide global-vs-local at the
        // point of emission. Lambdas are excluded — captureScopeVariables handles
        // them via the existing capture path. Imports stay globals (handled by
        // inImportedFile).
        {
            auto refs = analysis::NestedReferenceCollector::collect(root);
            context.nestedReferencesPessimistic = refs.pessimistic;
            context.namesReferencedByNestedNonLambdaFns = std::move(refs.names);
        }

        // Visit the root node to generate bytecode
        root->accept(*this);

        // Publish the top-level local count before tearing the frame down —
        // OSR needs this to tier-up loops at script scope (the top-level
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

        // Validate all class methods have bytecode implementations.
        // Skip in Release mode since the AST optimizer may have removed unused methods.
        if (!skipStrictValidation) {
            classRegistrar.validateAllClassesHaveBytecode(root);
        }

        // Emit halt instruction
        context.emitter.emitWithLocation(bytecode::OpCode::HALT, root);

        // Bytecode-level post-processing — runs the LoopOptimization, Peephole,
        // TrivialSetterInlining, TrivialGetterInlining, and LocalArrayFusion
        // passes in registration order. See mType/vm/optimization/ for the pipeline.
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

        if (auto* declNode = dynamic_cast<ast::AnnotationDeclarationNode*>(node))
        {
            visitAnnotationDeclarationNode(declNode);
            return;
        }

        // Recurse through container nodes that may hold top-level declarations
        // (the annotation keyword is only legal at top level).
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
        // Register interfaces FIRST so classes can validate against them
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

            const auto& genericParams = interfaceNode->getGenericParameters();
            for (const auto& param : genericParams) {
                meta.genericParameters.push_back(param.name);
            }

            meta.extendsInterfaces = interfaceNode->getExtendedInterfaces();

            for (const auto& method : interfaceNode->getMethods()) {
                if (auto* funcNode = dynamic_cast<ast::FunctionNode*>(method.get())) {
                    bytecode::BytecodeProgram::InterfaceMethodSignature sig;
                    sig.name = funcNode->getName();
                    sig.returnType = ::types::TypeConversionUtils::getTypeDisplayName(funcNode->getReturnType());

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
}
