#include "ClassRegistrar.hpp"
#include <cstddef>
#include "AnnotationRetention.hpp"
#include "../BytecodeCompiler.hpp"
#include "../validation/CompileTimeValidator.hpp"
#include "../../../analysis/OverrideAnnotationChecker.hpp"
#include "../../MethodSignature.hpp"
#include "../../../ast/nodes/statements/ProgramNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/ImportNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/InheritanceException.hpp"
#include "../../../errors/AbstractClassException.hpp"
#include "../../../errors/DuplicateSignatureException.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../types/TypeSubstitutionService.hpp"
#include "../../../types/TypeConversionBridge.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../../runtimeTypes/klass/SignatureUtils.hpp"
#include "../../../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../../../runtimeTypes/klass/FieldDefinition.hpp"
#include "../../../validation/AnnotationValidator.hpp"
#include "../../../ast/nodes/annotations/TypedAnnotationValue.hpp"
#include "../../../environment/registry/AnnotationRegistry.hpp"
#include "../../../runtimeTypes/klass/AnnotationDefinition.hpp"
#include <stdexcept>

namespace vm::compiler::registration
{
    namespace
    {
        // MYT-110: shouldRetainAnnotation and populateAnnotationData live in
        // AnnotationRetention.{hpp,cpp} — single source of truth shared with
        // FunctionRegistrar. Same-namespace call sites below resolve directly
        // to those symbols.
    }

    ClassRegistrar::ClassRegistrar(
        std::shared_ptr<environment::Environment> environment,
        bytecode::BytecodeProgram& program,
        InterfaceRegistrar* interfaceRegistrar
    )
        : environment(environment)
        , program(program)
        , interfaceRegistrar(interfaceRegistrar)
        , inheritanceValidator(std::make_unique<ClassInheritanceValidator>(environment->getClassRegistry()))
        , typeSubstitutionService(std::make_shared<::types::TypeSubstitutionService>())
    {
    }

    void ClassRegistrar::registerClassesForBytecode(ast::ASTNode* node)
    {
        if (!node) return;
        // Check if this node is a ClassNode
        if (auto classNode = dynamic_cast<ast::ClassNode*>(node))
        {
            registerSingleClass(classNode);
            return; // No need to traverse children of ClassNode
        }

        // Recursively process child nodes
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                registerClassesForBytecode(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                registerClassesForBytecode(statement.get());
            }
        }
        else if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            // Process classes from imported AST
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                registerClassesForBytecode(importNode->getImportedAST());
            }
        }
    }

    void ClassRegistrar::registerSingleClass(ast::ClassNode* classNode)
    {
        std::string className = classNode->getClassName();

        // Check if class already registered
        if (environment->findClass(className)) {
            return; // Already registered, skip
        }

        // Create class definition programmatically
        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            throw std::runtime_error("Class registry not available");
        }

        // Create a new class definition
        auto classDef = std::make_shared<runtimeTypes::klass::ClassDefinition>(className);

        // Set generic parameters (e.g., T in class Container<T>)
        classDef->setGenericParameters(classNode->getGenericParameters());

        // Set final modifier
        classDef->setFinal(classNode->isFinal());

        // Set abstract modifier
        classDef->setAbstract(classNode->isAbstract());

        // Set value class modifier
        classDef->setValueClass(classNode->isValueClass());

        // Copy annotations from AST to runtime definition (drop SOURCE-retention).
        for (const auto& annotation : classNode->getAnnotations()) {
            if (!annotation) continue;
            if (!shouldRetainAnnotation(*annotation, environment)) continue;
            classDef->addAnnotation(annotation);
        }

        // Handle parent class
        if (classNode->hasParentClass()) {
            const std::string& parentClassName = classNode->getParentClassName();

            // Note: Parser already validates that classes cannot extend interfaces
            // at parse time (ClassDeclarationParser.cpp:92-98)

            classDef->setParentClassName(parentClassName);
        }
        else if (className != "Object") {
            // Implicit Object inheritance: all classes without explicit parent inherit from Object
            classDef->setParentClassName("Object");
        }

        // Handle implemented interfaces
        const auto& interfaces = classNode->getImplementedInterfaces();
        for (const auto& interfaceName : interfaces) {
            classDef->addImplementedInterface(interfaceName);
        }
        // Register constructors
        if (!classNode->getConstructors().empty()) {
            for (const auto& constructor : classNode->getConstructors()) {
                if (auto* ctorNode = dynamic_cast<ast::nodes::classes::ConstructorNode*>(constructor.get())) {
                    auto ctorDef = std::make_shared<runtimeTypes::klass::ConstructorDefinition>(
                        ctorNode->getParametersWithTypes(),
                        ctorNode->getBody(),
                        ctorNode->getAccessModifier()
                    );
                    // MYT-108: copy constructor annotations to runtime definition
                    // (drop SOURCE-retention annotations per MYT-109).
                    for (const auto& annotation : ctorNode->getAnnotations()) {
                        if (!annotation) continue;
                        if (!shouldRetainAnnotation(*annotation, environment)) continue;
                        ctorDef->addAnnotation(annotation);
                    }
                    // MYT-110: copy per-parameter annotations (with retention filter).
                    // Constructor params in AST and runtime are 1:1 aligned.
                    {
                        const auto& astParamAnnots = ctorNode->getParameterAnnotations();
                        std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> filtered;
                        filtered.reserve(astParamAnnots.size());
                        for (const auto& perParam : astParamAnnots)
                        {
                            std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> kept;
                            for (const auto& a : perParam)
                            {
                                if (!a) continue;
                                if (!shouldRetainAnnotation(*a, environment)) continue;
                                kept.push_back(a);
                            }
                            filtered.push_back(std::move(kept));
                        }
                        ctorDef->setParameterAnnotations(std::move(filtered));
                    }
                    classDef->addConstructor(ctorDef);
                }
            }
        } else {
            // No explicit constructors - register a default constructor
            std::vector<std::pair<std::string, value::ParameterType>> emptyParams;
            auto defaultCtor = std::make_shared<runtimeTypes::klass::ConstructorDefinition>(
                emptyParams,
                nullptr  // No AST body - bytecode will handle it
            );
            classDef->addConstructor(defaultCtor);
        }
        // Register methods
        for (const auto& method : classNode->getMethods()) {
            if (auto* methodNode = dynamic_cast<ast::nodes::classes::MethodNode*>(method.get())) {
                // Convert generic parameters to ParameterType (with className information)
                std::vector<std::pair<std::string, value::ParameterType>> params;

                // For instance methods, add 'this' as the first parameter
                if (!methodNode->getIsStatic()) {
                    params.emplace_back("this", value::ParameterType::forClass(className));
                }

                for (const auto& [name, genericType] : methodNode->getGenericParameters()) {
                    if (!genericType) {
                        params.emplace_back(name, value::ParameterType(value::ValueType::VOID));
                        continue;
                    }

                    // Check if it's a "generic parameter" - but it could be a class name stored as string
                    if (genericType->isGenericParameter()) {
                        // This is stored as a string - could be:
                        // 1. An actual generic type parameter (T, K, V, etc.)
                        // 2. A class/interface name (String, Calculator, etc.)
                        // 3. A parameterized type (Container<Int>, Map<K,V>, etc.)
                        //
                        // IMPORTANT: Use toString() instead of getGenericName() to preserve
                        // generic type arguments (e.g., "Container<Int>" not just "Container")
                        std::string typeName = genericType->toString();

                        // Extract base type name (before '<' if it exists) for registry lookups
                        std::string baseTypeName = typeName;
                        size_t anglePos = typeName.find('<');
                        if (anglePos != std::string::npos) {
                            baseTypeName = typeName.substr(0, anglePos);
                        }

                        // IMPORTANT: Check if baseTypeName matches the class we're currently registering
                        // This handles self-referential types (e.g., String::equals(String other))
                        bool isSelfReference = (baseTypeName == className);

                        if (isSelfReference) {
                            // Self-reference to the class being registered
                            // Use full typeName to preserve generic arguments
                            params.emplace_back(name, value::ParameterType::forClass(typeName));
                        }
                        else {
                            // Check if the base type is a registered class or interface
                            auto classRegistry = environment->getClassRegistry();
                            auto interfaceRegistry = environment->getInterfaceRegistry();

                            bool isClass = classRegistry && classRegistry->findClass(baseTypeName);
                            bool isInterface = interfaceRegistry && interfaceRegistry->findInterface(baseTypeName);

                            if (isClass) {
                                // It's a class name (possibly parameterized)
                                // Use full typeName to preserve generic arguments (e.g., Container<Int>)
                                params.emplace_back(name, value::ParameterType::forClass(typeName));
                            }
                            else if (isInterface) {
                                // It's an interface name (possibly parameterized)
                                // Use full typeName to preserve generic arguments
                                params.emplace_back(name, value::ParameterType::forInterface(typeName));
                            }
                            else {
                                // It's an actual generic type parameter (T, K, etc.) or unknown type
                                // Store the full type name (which could include parameters)
                                // This allows the validator to match against bytecode function names
                                params.emplace_back(name, value::ParameterType::forClass(typeName));
                            }
                        }
                    }
                    else {
                        // Concrete primitive type
                        value::ValueType concreteType = genericType->getConcreteType();
                        params.emplace_back(name, value::ParameterType(concreteType));
                    }
                }

                // Use the constructor that takes ParameterType directly (preserves className)
                auto methodDef = std::make_shared<runtimeTypes::klass::MethodDefinition>(
                    methodNode->getName(),
                    methodNode->getReturnType(),
                    params,  // Use ParameterType vector instead of ValueType
                    methodNode->getBody(),
                    methodNode->getIsStatic(),
                    methodNode->getAccessModifier()
                );

                // Set source location for error reporting
                methodDef->setSourceLocation(methodNode->getLocation());

                // Set unified type information for proper interface validation
                if (methodNode->getGenericReturnType()) {
                    methodDef->setUnifiedReturnType(
                        ::types::TypeConversionBridge::toUnifiedType(methodNode->getGenericReturnType()));
                }
                {
                    std::vector<std::pair<std::string, ::types::UnifiedTypePtr>> convertedParams;
                    for (const auto& [pName, genType] : methodNode->getGenericParameters()) {
                        convertedParams.emplace_back(pName,
                            ::types::TypeConversionBridge::toUnifiedType(genType));
                    }
                    methodDef->setUnifiedParameters(convertedParams);
                }
                methodDef->setGenericTypeParameters(methodNode->getGenericTypeParameters());

                // Set abstract flag
                methodDef->setAbstract(methodNode->isAbstract());
                if (methodNode->isAbstract()) {
                    classDef->addAbstractMethod(methodNode->getName());
                }

                // Set final flag
                methodDef->setFinal(methodNode->isFinal());

                // MYT-113: propagate the async flag so VM::invokeMethod /
                // invokeStaticMethod can take the Promise-returning interop
                // path for async targets (e.g. mtest reflectively invoking
                // an `async function testX(): Promise<void>`).
                methodDef->setIsAsync(methodNode->getIsAsync());

                // MYT-274: propagate synthetic flag so reflection can filter
                // compiler-generated hashCode/equals out of getDeclaredMethods().
                methodDef->setSynthetic(methodNode->isSynthetic());

                // Copy annotations from AST to runtime definition
                // (drop SOURCE-retention per MYT-109).
                for (const auto& annotation : methodNode->getAnnotations()) {
                    if (!annotation) continue;
                    if (!shouldRetainAnnotation(*annotation, environment)) continue;
                    methodDef->addAnnotation(annotation);
                }

                // MYT-110: copy per-parameter annotations (with retention
                // filter). Instance methods prepend an implicit `this` slot at
                // runtime-param index 0 that has no annotations in the AST.
                {
                    const auto& astParamAnnots = methodNode->getParameterAnnotations();
                    std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> filtered;
                    const size_t runtimeParamCount = methodDef->getParameters().size();
                    filtered.reserve(runtimeParamCount);
                    const bool hasThis = !methodNode->getIsStatic();
                    if (hasThis)
                    {
                        filtered.push_back({}); // empty slot for `this`
                    }
                    for (const auto& perParam : astParamAnnots)
                    {
                        std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> kept;
                        for (const auto& a : perParam)
                        {
                            if (!a) continue;
                            if (!shouldRetainAnnotation(*a, environment)) continue;
                            kept.push_back(a);
                        }
                        filtered.push_back(std::move(kept));
                    }
                    methodDef->setParameterAnnotations(std::move(filtered));
                }

                // Check for duplicate signatures (overloading by name is now allowed, but signatures must be unique)
                if (methodNode->getIsStatic()) {
                    // Check if this exact signature already exists
                    auto existingMethod = classDef->findStaticMethodBySignature(
                        methodNode->getName(),
                        methodDef->getParameters()
                    );
                    if (existingMethod) {
                        throw errors::DuplicateSignatureException(
                            "static method",
                            methodNode->getName(),
                            runtimeTypes::klass::SignatureUtils::generateTypeSignature(methodDef->getParameters()),
                            existingMethod->getSourceLocation(),
                            methodNode->getLocation()
                        );
                    }
                    // Register with original name for overload tracking
                    classDef->addStaticMethod(methodNode->getName(), methodDef);
                } else {
                    // Check if this exact signature already exists
                    auto existingMethod = classDef->findInstanceMethodBySignature(
                        methodNode->getName(),
                        methodDef->getParameters()
                    );
                    if (existingMethod) {
                        // For instance methods, skip the 'this' parameter (first parameter) when generating signature for error message
                        auto paramsWithoutThis = methodDef->getParameters();
                        if (!paramsWithoutThis.empty() && paramsWithoutThis[0].first == "this") {
                            paramsWithoutThis.erase(paramsWithoutThis.begin());
                        }

                        throw errors::DuplicateSignatureException(
                            "method",
                            methodNode->getName(),
                            runtimeTypes::klass::SignatureUtils::generateTypeSignature(paramsWithoutThis),
                            existingMethod->getSourceLocation(),
                            methodNode->getLocation()
                        );
                    }
                    // Register with original name for overload tracking
                    classDef->addMethod(methodDef);
                }
            }
        }
        // Register fields
        for (const auto& field : classNode->getFields()) {
            if (auto* fieldNode = dynamic_cast<ast::nodes::classes::FieldNode*>(field.get())) {
                // Initialize static fields with default values based on type
                value::Value defaultValue;
                if (fieldNode->getIsStatic())
                {
                    value::ValueType fieldType = fieldNode->getType();
                    switch (fieldType)
                    {
                    case value::ValueType::INT:
                        defaultValue = 0;
                        break;
                    case value::ValueType::FLOAT:
                        defaultValue = 0.0;
                        break;
                    case value::ValueType::STRING:
                        defaultValue = std::string("");
                        break;
                    case value::ValueType::BOOL:
                        defaultValue = false;
                        break;
                    default:
                        // Objects, arrays, lambdas initialize to null
                        defaultValue = nullptr;
                        break;
                    }
                }
                else
                {
                    // Instance fields use monostate as sentinel - ObjectInstanceHelper applies proper type defaults
                    defaultValue = std::monostate{};
                }

                auto fieldDef = std::make_shared<runtimeTypes::klass::FieldDefinition>(
                    fieldNode->getName(),
                    fieldNode->getType(),
                    ::types::TypeConversionBridge::toUnifiedType(fieldNode->getGenericType()),
                    defaultValue,
                    fieldNode->getIsStatic(),
                    fieldNode->getIsFinal(),
                    fieldNode->getAccessModifier()
                );

                // MYT-108: copy field annotations to runtime definition
                // (drop SOURCE-retention per MYT-109).
                for (const auto& annotation : fieldNode->getAnnotations()) {
                    if (!annotation) continue;
                    if (!shouldRetainAnnotation(*annotation, environment)) continue;
                    fieldDef->addAnnotation(annotation);
                }

                if (fieldNode->getIsStatic()) {
                    classDef->addStaticField(fieldNode->getName(), fieldDef);
                } else {
                    classDef->addInstanceField(fieldNode->getName(), fieldDef);
                }
            }
        }
        // Register the class
        classRegistry->registerClass(className, classDef);

        // Validate interface implementations
        if (interfaceRegistrar && !classDef->getImplementedInterfaces().empty()) {
            interfaceRegistrar->validateInterfaceImplementations(classDef, classNode);
        }

        // Note: Abstract method validation and annotation validation are done in linkSingleClass()
        // after parent links are established

        // Extract and store class metadata for bytecode serialization
        auto classMetadata = extractClassMetadata(classNode);
        program.registerClass(classMetadata);
    }

    void ClassRegistrar::validateAllClassesHaveBytecode(ast::ASTNode* node)
    {
        if (!node || !compileTimeValidator) return;

        // Check if this node is a ClassNode
        if (auto classNode = dynamic_cast<ast::ClassNode*>(node))
        {
            compileTimeValidator->validateAllMethodsHaveBytecode(
                classNode->getClassName(),
                classNode->getLocation()
            );
            return; // No need to traverse children of ClassNode
        }

        // Recursively process child nodes
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                validateAllClassesHaveBytecode(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                validateAllClassesHaveBytecode(statement.get());
            }
        }
        else if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            // Process classes from imported AST
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                validateAllClassesHaveBytecode(importNode->getImportedAST());
            }
        }
    }

    void ClassRegistrar::linkParentClasses(ast::ASTNode* node)
    {
        if (!node) return;

        // Check if this node is a ClassNode with a parent
        if (auto classNode = dynamic_cast<ast::ClassNode*>(node))
        {
            linkSingleClass(classNode);
            return; // No need to traverse children of ClassNode
        }

        // Recursively process child nodes
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                linkParentClasses(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                linkParentClasses(statement.get());
            }
        }
        else if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            // Process classes from imported AST
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                linkParentClasses(importNode->getImportedAST());
            }
        }
    }

    void ClassRegistrar::linkSingleClass(ast::ClassNode* classNode)
    {
        std::string className = classNode->getClassName();
        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            throw std::runtime_error("Class registry not available");
        }

        // Handle classes without explicit parent in AST
        if (!classNode->hasParentClass()) {
            if (className == "Object") {
                // Object is the root — no parent to link, just validate
                auto classDef = classRegistry->findClass(className);
                if (classDef && !classDef->isAbstract()) {
                    auto unimplemented = classDef->getUnimplementedAbstractMethods();
                    if (!unimplemented.empty()) {
                        std::string methodList;
                        for (size_t i = 0; i < unimplemented.size(); ++i) {
                            if (i > 0) methodList += ", ";
                            methodList += unimplemented[i];
                        }
                        throw errors::AbstractClassException(
                            "Concrete class '" + className + "' cannot have abstract methods: " + methodList,
                            classNode->getLocation()
                        );
                    }
                }
                return;
            }

            // Implicit Object parent — link it
            auto classDef = classRegistry->findClass(className);
            auto parentDef = classRegistry->findClass("Object");
            if (classDef && parentDef) {
                classDef->setParentClass(parentDef);

                // Validate annotations (e.g., @Override now valid against Object methods)
                ::validation::AnnotationValidator::validateClassAnnotations(classDef, environment);

                // Validate method overrides against Object
                validateMethodOverrides(classDef, parentDef, classNode);

                // MYT-50 — also run the missing-@Override checker for
                // classes whose parent is the implicit Object root.
                if (compiler_) {
                    auto warns = analysis::OverrideAnnotationChecker::check(*classDef);
                    for (auto& w : warns) {
                        compiler_->addWarning(std::move(w));
                    }
                }

                // Validate abstract method implementations
                if (!classDef->isAbstract()) {
                    auto unimplemented = classDef->getUnimplementedAbstractMethods();
                    if (!unimplemented.empty()) {
                        std::string methodList;
                        for (size_t i = 0; i < unimplemented.size(); ++i) {
                            if (i > 0) methodList += ", ";
                            methodList += unimplemented[i];
                        }
                        throw errors::AbstractClassException(
                            "Concrete class '" + className + "' cannot have abstract methods: " + methodList,
                            classNode->getLocation()
                        );
                    }
                }
            }
            return;
        }

        std::string parentClassName = classNode->getParentClassName();

        // Strip generic type parameters from parent class name: "Container<T>" -> "Container"
        size_t genericStart = parentClassName.find('<');
        std::string baseParentClassName = (genericStart != std::string::npos)
            ? parentClassName.substr(0, genericStart)
            : parentClassName;

        // Validate parent class exists
        validateParentClassExists(baseParentClassName, classNode->getLocation());

        // Get both class definitions
        auto classDef = classRegistry->findClass(className);
        auto parentDef = classRegistry->findClass(baseParentClassName);

        if (classDef && parentDef) {
            // Check for circular inheritance before establishing the link
            // Note: Parser validates circular inheritance within single files (ClassDeclarationParser.cpp:109-124)
            // This check remains as a safety net for cross-file import scenarios
            checkCircularInheritance(className, parentClassName, parentDef);

            // Note: Parser already validates that classes cannot extend final classes
            // at parse time (ClassDeclarationParser.cpp:101-106)

            // Establish the parent-child link
            classDef->setParentClass(parentDef);

            // Parse generic type arguments and create substitution map
            // E.g., "Processor<String>" with parent having generic param "T" creates mapping: T → String
            if (genericStart != std::string::npos) {
                parseAndStoreTypeSubstitutions(classDef, parentClassName, parentDef);
            }

            // Validate inheritance depth after establishing the link
            validateInheritanceDepth(className, classNode->getLocation());

            // Validate annotations (e.g., @Override) after parent link is established
            ::validation::AnnotationValidator::validateClassAnnotations(classDef, environment);

            // Validate method overrides
            validateMethodOverrides(classDef, parentDef, classNode);

            // MYT-50 — emit MT-W2002 warnings for methods that override
            // an ancestor without an explicit @Override annotation. The
            // checker is read-only and skips classes with no parent;
            // safe to call here once the parent link is established.
            if (compiler_) {
                auto warns = analysis::OverrideAnnotationChecker::check(*classDef);
                for (auto& w : warns) {
                    compiler_->addWarning(std::move(w));
                }
            }

            // Validate abstract method implementations (must be done after parent link is established)
            if (!classDef->isAbstract()) {
                // Concrete classes must implement all abstract methods from parent chain
                auto unimplemented = classDef->getUnimplementedAbstractMethods();
                if (!unimplemented.empty()) {
                    std::string methodList;
                    for (size_t i = 0; i < unimplemented.size(); ++i) {
                        if (i > 0) methodList += ", ";
                        methodList += unimplemented[i];
                    }
                    throw errors::AbstractClassException(
                        "Concrete class '" + className + "' must implement all abstract methods: " + methodList,
                        classNode->getLocation()
                    );
                }
            }
        }
    }

    void ClassRegistrar::checkCircularInheritance(
        const std::string& className,
        const std::string& parentClassName,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentDef
    ) const
    {
        std::unordered_set<std::string> visited;
        auto currentCheck = parentDef;
        int depth = 0;
        while (currentCheck) {
            std::string checkName = currentCheck->getName();
            if (depth++ > 100) {
                throw errors::RuntimeException(
                    "Circular inheritance loop detected (depth > 100): class '" + className +
                    "' cannot extend '" + parentClassName + "'"
                );
            }
            if (visited.count(checkName)) {
                // Circular inheritance detected
                throw errors::RuntimeException(
                    "Circular inheritance detected: class '" + className +
                    "' cannot extend '" + parentClassName + "'"
                );
            }
            if (checkName == className) {
                // Parent's ancestor is the current class - circular!
                throw errors::RuntimeException(
                    "Circular inheritance detected: class '" + className +
                    "' cannot extend '" + parentClassName + "'"
                );
            }
            visited.insert(checkName);
            currentCheck = currentCheck->getParentClass();
        }
    }

    bytecode::BytecodeProgram::ClassMetadata ClassRegistrar::extractClassMetadata(ast::ClassNode* classNode) const
    {
        bytecode::BytecodeProgram::ClassMetadata metadata;

        // Extract basic class information
        metadata.name = classNode->getClassName();
        metadata.parentClassName = classNode->hasParentClass() ? classNode->getParentClassName() : "";
        metadata.isAbstract = classNode->isAbstract();
        metadata.isFinal = classNode->isFinal();
        metadata.isValueClass = classNode->isValueClass();

        // Extract implemented interfaces
        const auto& interfaces = classNode->getImplementedInterfaces();
        for (const auto& iface : interfaces) {
            metadata.implementedInterfaces.push_back(iface);
        }

        // Extract generic parameters
        const auto& genericParams = classNode->getGenericParameters();
        for (const auto& param : genericParams) {
            metadata.genericParameters.push_back(param.name);
        }

        // Extract annotations (MYT-108 typed-args; SOURCE-retention stripped per MYT-109)
        for (const auto& annotationNode : classNode->getAnnotations()) {
            if (!annotationNode) continue;
            if (!shouldRetainAnnotation(*annotationNode, environment)) continue;
            bytecode::BytecodeProgram::AnnotationData annot;
            populateAnnotationData(annot, *annotationNode);
            metadata.annotations.push_back(std::move(annot));
        }

        // Extract fields
        const auto& fields = classNode->getFields();
        for (const auto& fieldNode : fields) {
            auto* field = dynamic_cast<ast::FieldNode*>(fieldNode.get());
            if (!field) continue;

            bytecode::BytecodeProgram::FieldMetadata fieldMeta;
            fieldMeta.name = field->getName();
            fieldMeta.type = ::types::TypeConversionUtils::getTypeDisplayName(field->getType());
            fieldMeta.isStatic = field->getIsStatic();
            fieldMeta.isFinal = field->getIsFinal();

            // Get access modifiers
            auto accessMod = field->getAccessModifier();
            fieldMeta.isPrivate = (accessMod == ast::AccessModifier::PRIVATE);
            fieldMeta.isProtected = (accessMod == ast::AccessModifier::PROTECTED);

            // MYT-108: copy field annotations into metadata (typed-args)
            // SOURCE-retention annotations are stripped per MYT-109.
            for (const auto& annotationNode : field->getAnnotations()) {
                if (!annotationNode) continue;
                if (!shouldRetainAnnotation(*annotationNode, environment)) continue;
                bytecode::BytecodeProgram::AnnotationData annot;
                populateAnnotationData(annot, *annotationNode);
                fieldMeta.annotations.push_back(std::move(annot));
            }

            if (field->getIsStatic()) {
                metadata.staticFields.push_back(fieldMeta);
            } else {
                metadata.instanceFields.push_back(fieldMeta);
            }
        }

        // Extract methods
        const auto& methods = classNode->getMethods();
        for (const auto& methodNode : methods) {
            auto* method = dynamic_cast<ast::MethodNode*>(methodNode.get());
            if (!method) continue;

            bytecode::BytecodeProgram::MethodMetadata methodMeta;
            methodMeta.name = method->getName();
            methodMeta.returnType = ::types::TypeConversionUtils::getTypeDisplayName(method->getReturnType());
            methodMeta.isStatic = method->getIsStatic();
            methodMeta.isFinal = method->isFinal();  // Support final method modifier

            // Get access modifiers
            auto accessMod = method->getAccessModifier();
            methodMeta.isPrivate = (accessMod == ast::AccessModifier::PRIVATE);
            methodMeta.isProtected = (accessMod == ast::AccessModifier::PROTECTED);
            methodMeta.isAbstract = method->isAbstract();
            methodMeta.startOffset = 0;  // Will be set during bytecode generation if needed

            // Extract parameter types and names (from AST MethodNode)
            const auto& genericParams = method->getGenericParameters();
            for (const auto& [paramName, genType] : genericParams) {
                value::ValueType vType = value::ValueType::VOID;
                if (genType) {
                    auto uType = ::types::TypeConversionBridge::toUnifiedType(genType);
                    vType = uType->isGenericParameter() ? value::ValueType::OBJECT : uType->toValueType();
                }
                methodMeta.parameterTypes.push_back(::types::TypeConversionUtils::getTypeDisplayName(vType));
                methodMeta.parameterNames.push_back(paramName);
            }

            // MYT-108: copy method annotations into metadata (typed-args)
            // SOURCE-retention annotations are stripped per MYT-109.
            for (const auto& annotationNode : method->getAnnotations()) {
                if (!annotationNode) continue;
                if (!shouldRetainAnnotation(*annotationNode, environment)) continue;
                bytecode::BytecodeProgram::AnnotationData annot;
                populateAnnotationData(annot, *annotationNode);
                methodMeta.annotations.push_back(std::move(annot));
            }

            // MYT-110: per-parameter annotations, parallel to AST parameters.
            const auto& methodParamAnnots = method->getParameterAnnotations();
            methodMeta.parameterAnnotations.resize(genericParams.size());
            for (size_t pi = 0; pi < genericParams.size(); ++pi) {
                if (pi >= methodParamAnnots.size()) continue;
                for (const auto& annotationNode : methodParamAnnots[pi]) {
                    if (!annotationNode) continue;
                    if (!shouldRetainAnnotation(*annotationNode, environment)) continue;
                    bytecode::BytecodeProgram::AnnotationData annot;
                    populateAnnotationData(annot, *annotationNode);
                    methodMeta.parameterAnnotations[pi].push_back(std::move(annot));
                }
            }

            if (method->getIsStatic()) {
                metadata.staticMethods.push_back(methodMeta);
            } else {
                metadata.instanceMethods.push_back(methodMeta);
            }
        }

        // Extract constructors
        const auto& constructors = classNode->getConstructors();
        for (const auto& ctorNode : constructors) {
            auto* ctor = dynamic_cast<ast::ConstructorNode*>(ctorNode.get());
            if (!ctor) continue;

            bytecode::BytecodeProgram::ConstructorMetadata ctorMeta;
            ctorMeta.startOffset = 0;  // Will be set during bytecode generation if needed

            // Extract parameter types and names from ParametersWithTypes
            const auto& params = ctor->getParametersWithTypes();
            for (const auto& param : params) {
                ctorMeta.parameterNames.push_back(param.first);
                ctorMeta.parameterTypes.push_back(param.second.toString());
            }

            // MYT-108: copy constructor annotations into metadata (typed-args)
            // SOURCE-retention annotations are stripped per MYT-109.
            for (const auto& annotationNode : ctor->getAnnotations()) {
                if (!annotationNode) continue;
                if (!shouldRetainAnnotation(*annotationNode, environment)) continue;
                bytecode::BytecodeProgram::AnnotationData annot;
                populateAnnotationData(annot, *annotationNode);
                ctorMeta.annotations.push_back(std::move(annot));
            }

            // MYT-110: per-parameter annotations
            const auto& ctorParamAnnots = ctor->getParameterAnnotations();
            ctorMeta.parameterAnnotations.resize(params.size());
            for (size_t pi = 0; pi < params.size(); ++pi) {
                if (pi >= ctorParamAnnots.size()) continue;
                for (const auto& annotationNode : ctorParamAnnots[pi]) {
                    if (!annotationNode) continue;
                    if (!shouldRetainAnnotation(*annotationNode, environment)) continue;
                    bytecode::BytecodeProgram::AnnotationData annot;
                    populateAnnotationData(annot, *annotationNode);
                    ctorMeta.parameterAnnotations[pi].push_back(std::move(annot));
                }
            }

            metadata.constructors.push_back(ctorMeta);
        }

        return metadata;
    }

    void ClassRegistrar::validateParentClassExists(
        const std::string& parentClassName,
        const ast::SourceLocation& location
    ) const
    {
        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            throw std::runtime_error("Class registry not available");
        }

        // Check if parent class exists
        auto parentClass = classRegistry->findClass(parentClassName);
        if (!parentClass) {
            throw errors::InheritanceException(
                "Parent class '" + parentClassName + "' does not exist",
                location
            );
        }
    }

    void ClassRegistrar::validateInheritanceDepth(
        const std::string& className,
        const ast::SourceLocation& location
    ) const
    {
        inheritanceValidator->validateInheritanceDepth(className, location);
    }

    void ClassRegistrar::validateMethodOverrides(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> childClass,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentClass,
        ast::ClassNode* classNode
    ) const
    {
        if (!childClass || !parentClass) {
            return;
        }

        // Use class location as fallback
        const ast::SourceLocation classLocation = classNode ? classNode->getLocation() : ast::SourceLocation();

        // Check each method in child class
        const auto& childMethods = childClass->getInstanceMethods();

        // Cache Object class lookup for invariance checks (Object is always the root)
        auto classRegistry = environment->getClassRegistry();
        auto objectClassDef = classRegistry ? classRegistry->findClass("Object") : nullptr;

        for (const auto& [methodName, childMethodOverloads] : childMethods) {
            // Check each overload
            for (const auto& childMethod : childMethodOverloads) {
                const auto& childParams = childMethod->getParameters();

                // IMPORTANT: Exclude 'this' parameter when comparing signatures for override validation
                // Instance methods have an implicit 'this' parameter with type equal to the class name
                // Parent and child have different 'this' types (Base vs Derived), so we must exclude it
                std::vector<std::pair<std::string, value::ParameterType>> paramsWithoutThis;
                for (const auto& param : childParams) {
                    if (param.first != "this") {
                        paramsWithoutThis.push_back(param);
                    }
                }

                // Search for method in parent hierarchy WITH EXACT SIGNATURE MATCH (excluding 'this' parameter)
                auto parentMethod = parentClass->findInstanceMethodBySignatureInHierarchy(
                    methodName,
                    paramsWithoutThis
                );

                // Check if parent has ANY method with this name (for invariance validation)
                bool parentHasMethodName = false;
                auto parentMethodsIt = parentClass->getInstanceMethods().find(methodName);
                if (parentMethodsIt != parentClass->getInstanceMethods().end() && !parentMethodsIt->second.empty()) {
                    parentHasMethodName = true;
                }

                // PARAMETER TYPE INVARIANCE RULE:
                // If a parent method exists with the same name and same arity as this child method,
                // but with different parameter types, we need to check if the parent method is being
                // overridden by ANY child method.
                //
                // Cases:
                //   1. Parent: process(string), Child: process(int) with NO process(string) override
                //      → ERROR: invariance violation (looks like failed override)
                //   2. Parent: compute(int), Child: compute(int) AND compute(string)
                //      → OK: compute(int) overrides parent, compute(string) is new overload
                //
                if (parentHasMethodName && !parentMethod) {
                    // Skip invariance check for Object base class methods (toString, equals, hashCode)
                    // These are intentionally designed to be overloadable with typed versions
                    // e.g., equals(Int other) is a valid overload alongside equals(Object other)
                    if (objectClassDef) {
                        auto objectMethods = objectClassDef->getInstanceMethods().find(methodName);
                        if (objectMethods != objectClassDef->getInstanceMethods().end()) {
                            continue; // Allow typed overloads of Object methods
                        }
                    }

                    // Get parent methods with this name
                    auto& parentMethodOverloads = parentMethodsIt->second;

                    // Count child parameters (excluding 'this')
                    size_t childArity = paramsWithoutThis.size();

                    // For each parent method with same arity, check if it's being overridden by ANY child method
                    for (const auto& pMethod : parentMethodOverloads) {
                        const auto& pParams = pMethod->getParametersWithTypes();

                        // Build parent signature (excluding 'this')
                        std::vector<std::pair<std::string, value::ParameterType>> parentParamsWithoutThis;
                        for (const auto& [pName, pType] : pParams) {
                            if (pName != "this") {
                                parentParamsWithoutThis.push_back({pName, pType});
                            }
                        }

                        size_t parentArity = parentParamsWithoutThis.size();

                        // Only check invariance if arity matches this child method
                        if (parentArity == childArity) {
                            // Check if types match (accounting for generic type substitution)
                            bool typesMatch = true;
                            bool isGenericSubstitution = false;

                            // Get the type substitution map (e.g., T → String)
                            const auto& typeSubstitutionMap = childClass->getParentTypeSubstitutionMap();

                            for (size_t i = 0; i < parentArity; ++i) {
                                std::string pTypeStr = parentParamsWithoutThis[i].second.className.has_value()
                                    ? parentParamsWithoutThis[i].second.className.value()
                                    : runtimeTypes::klass::SignatureUtils::getTypeName(parentParamsWithoutThis[i].second);

                                std::string cTypeStr = paramsWithoutThis[i].second.className.has_value()
                                    ? paramsWithoutThis[i].second.className.value()
                                    : runtimeTypes::klass::SignatureUtils::getTypeName(paramsWithoutThis[i].second);

                                if (pTypeStr != cTypeStr) {
                                    typesMatch = false;

                                    // Check if this is a generic type substitution
                                    // e.g., parent has "T", substitution map has T→String, child has "String"
                                    auto substitutionIt = typeSubstitutionMap.find(pTypeStr);
                                    if (substitutionIt != typeSubstitutionMap.end()) {
                                        if (substitutionIt->second == cTypeStr) {
                                            // This is a valid generic substitution
                                            isGenericSubstitution = true;
                                        }
                                    }

                                    // Also check if parent type contains generic parameters (e.g., "Container<T>")
                                    if (pTypeStr.find('<') != std::string::npos) {
                                        isGenericSubstitution = true;
                                    }
                                }
                            }

                            // If types don't match and it's not a generic substitution, check if ANY child method overrides this parent method
                            if (!typesMatch && !isGenericSubstitution) {
                                // Check if this parent method is overridden by ANY child method
                                bool isOverriddenByAnyChild = false;

                                // Get all child methods with this name
                                auto childMethodsIt = childClass->getInstanceMethods().find(methodName);
                                if (childMethodsIt != childClass->getInstanceMethods().end()) {
                                    for (const auto& childMethodOverload : childMethodsIt->second) {
                                        const auto& childOverloadParams = childMethodOverload->getParametersWithTypes();

                                        // Build signature without 'this'
                                        std::vector<std::pair<std::string, value::ParameterType>> childOverloadParamsWithoutThis;
                                        for (const auto& [cName, cType] : childOverloadParams) {
                                            if (cName != "this") {
                                                childOverloadParamsWithoutThis.push_back({cName, cType});
                                            }
                                        }

                                        // Check if this child overload matches the parent method
                                        if (childOverloadParamsWithoutThis.size() == parentParamsWithoutThis.size()) {
                                            bool allMatch = true;
                                            for (size_t i = 0; i < parentParamsWithoutThis.size(); ++i) {
                                                if (!(childOverloadParamsWithoutThis[i].second == parentParamsWithoutThis[i].second)) {
                                                    allMatch = false;
                                                    break;
                                                }
                                            }

                                            if (allMatch) {
                                                isOverriddenByAnyChild = true;
                                                break;
                                            }
                                        }
                                    }
                                }

                                // If parent method is not overridden by any child method, it's an invariance violation
                                if (!isOverriddenByAnyChild) {
                                    // Find the specific method node to get its location
                                    ast::SourceLocation methodLocation = classLocation;
                                    if (classNode) {
                                        const auto& methods = classNode->getMethods();
                                        for (const auto& methodNodePtr : methods) {
                                            if (auto methodNode = dynamic_cast<ast::nodes::classes::MethodNode*>(methodNodePtr.get())) {
                                                if (methodNode->getName() == methodName) {
                                                    methodLocation = methodNode->getLocation();
                                                    break;
                                                }
                                            }
                                        }
                                    }

                                    throw errors::InheritanceException(
                                        "Method parameter type invariance violation in class '" + childClass->getName() +
                                        "': method '" + methodName + "' has the same number of parameters as a parent method in '" +
                                        parentClass->getName() + "' but with different parameter types, and the parent method is not being overridden. " +
                                        "This appears to be a failed override attempt. Method parameters are invariant and must match exactly when overriding.",
                                        childClass->getName(),
                                        parentClass->getName(),
                                        methodName,
                                        methodLocation
                                    );
                                }
                            }
                        }
                    }
                }

                if (parentMethod) {
                // Method exists in parent hierarchy - validate override

                // Find the specific method node to get its location
                ast::SourceLocation methodLocation = classLocation;
                if (classNode) {
                    const auto& methods = classNode->getMethods();
                    for (const auto& methodNodePtr : methods) {
                        if (auto methodNode = dynamic_cast<ast::nodes::classes::MethodNode*>(methodNodePtr.get())) {
                            if (methodNode->getName() == methodName) {
                                methodLocation = methodNode->getLocation();
                                break;
                            }
                        }
                    }
                }

                // Check if parent method is final (cannot be overridden)
                if (parentMethod->isFinal()) {
                    // Find which class in the hierarchy actually defines this final method
                    std::string definingClassName = parentClass->getName();
                    auto currentClass = parentClass;
                    while (currentClass) {
                        auto localMethod = currentClass->findInstanceMethodBySignature(methodName, paramsWithoutThis);
                        if (localMethod) {
                            definingClassName = currentClass->getName();
                            break;
                        }
                        currentClass = currentClass->getParentClass();
                    }

                    throw errors::InheritanceException(
                        "Cannot override final method '" + methodName +
                        "' from class '" + definingClassName + "' in class '" + childClass->getName() + "'",
                        childClass->getName(),
                        definingClassName,
                        methodName,
                        methodLocation
                    );
                }

                // Validate access modifier - child cannot narrow parent's access
                ast::AccessModifier childAccess = childMethod->getAccessModifier();
                ast::AccessModifier parentAccess = parentMethod->getAccessModifier();

                // Check for access modifier narrowing
                bool isNarrowing = false;
                if (parentAccess == ast::AccessModifier::PUBLIC && childAccess != ast::AccessModifier::PUBLIC) {
                    isNarrowing = true; // PUBLIC → PROTECTED or PRIVATE is narrowing
                } else if (parentAccess == ast::AccessModifier::PROTECTED && childAccess == ast::AccessModifier::PRIVATE) {
                    isNarrowing = true; // PROTECTED → PRIVATE is narrowing
                }

                if (isNarrowing) {
                    throw errors::InheritanceException(
                        "Method override access modifier narrowing in class '" + childClass->getName() +
                        "': method '" + methodName + "' cannot narrow access from '" +
                        ast::accessModifierToString(parentAccess) + "' to '" +
                        ast::accessModifierToString(childAccess) + "'",
                        childClass->getName(),
                        parentClass->getName(),
                        methodName,
                        methodLocation
                    );
                }

                // Validate parameter count matches
                const auto& childParams = childMethod->getParameters();
                const auto& parentParams = parentMethod->getParameters();

                if (childParams.size() != parentParams.size()) {
                    throw errors::InheritanceException(
                        "Method override signature mismatch in class '" + childClass->getName() +
                        "': method '" + methodName + "' has " + std::to_string(childParams.size()) +
                        " parameters but parent method has " + std::to_string(parentParams.size()) + " parameters",
                        childClass->getName(),
                        parentClass->getName(),
                        methodName,
                        methodLocation
                    );
                }

                // Validate parameter types match
                // Skip 'this' parameter (index 0) - it's always different between parent and child
                const auto& typeSubstitutionMap = childClass->getParentTypeSubstitutionMap();

                for (size_t i = 1; i < childParams.size(); ++i) {
                    const auto& childParam = childParams[i].second;
                    const auto& parentParam = parentParams[i].second;

                    // Apply type substitution to parent parameter type
                    // E.g., if parent has param "T" and substitution map is {T → String}, use "String" for comparison
                    std::string parentParamTypeName = parentParam.toString();
                    auto it = typeSubstitutionMap.find(parentParamTypeName);
                    if (it != typeSubstitutionMap.end()) {
                        parentParamTypeName = it->second;
                    }

                    // Compare child parameter type with substituted parent parameter type
                    std::string childParamTypeName = childParam.toString();

                    if (childParamTypeName != parentParamTypeName) {
                        throw errors::InheritanceException(
                            "Method override parameter type mismatch in class '" + childClass->getName() +
                            "': method '" + methodName + "' parameter " + std::to_string(i) +  // i, not i+1 (since we skip this)
                            " has type '" + childParamTypeName +
                            "' but parent method expects '" + parentParamTypeName + "'",
                            childClass->getName(),
                            parentClass->getName(),
                            methodName,
                            methodLocation
                        );
                    }
                }

                // Validate return type compatibility
                if (childMethod->getReturnType() != parentMethod->getReturnType()) {
                    throw errors::InheritanceException(
                        "Method override return type mismatch in class '" + childClass->getName() +
                        "': method '" + methodName + "' has return type '" +
                        ::types::TypeConversionUtils::getTypeDisplayName(childMethod->getReturnType()) +
                        "' but parent method has return type '" +
                        ::types::TypeConversionUtils::getTypeDisplayName(parentMethod->getReturnType()) + "'",
                        childClass->getName(),
                        parentClass->getName(),
                        methodName,
                        methodLocation
                    );
                }
            }
            }  // End of inner loop over childMethod overloads
        }  // End of outer loop over method names
    }

    std::vector<std::string> ClassRegistrar::parseGenericTypeArguments(const std::string& classNameWithGenerics) const
    {
        std::vector<std::string> typeArgs;

        // Find the opening angle bracket
        size_t start = classNameWithGenerics.find('<');
        if (start == std::string::npos) {
            return typeArgs;  // No generic arguments
        }

        // Find the matching closing bracket
        size_t end = classNameWithGenerics.rfind('>');
        if (end == std::string::npos || end <= start) {
            return typeArgs;  // Malformed
        }

        // Extract the content between < and >
        std::string argsStr = classNameWithGenerics.substr(start + 1, end - start - 1);

        // Parse comma-separated type arguments, respecting nested generics
        std::string current;
        int depth = 0;

        for (char c : argsStr) {
            if (c == '<') {
                depth++;
                current += c;
            } else if (c == '>') {
                depth--;
                current += c;
            } else if (c == ',' && depth == 0) {
                // Found a separator at the top level
                // Trim whitespace from current
                size_t firstNonSpace = current.find_first_not_of(" \t");
                size_t lastNonSpace = current.find_last_not_of(" \t");
                if (firstNonSpace != std::string::npos) {
                    typeArgs.push_back(current.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1));
                }
                current.clear();
            } else if (c != ' ' && c != '\t') {
                // Skip leading whitespace
                if (!current.empty() || (c != ' ' && c != '\t')) {
                    current += c;
                }
            } else if (!current.empty()) {
                // Preserve internal whitespace
                current += c;
            }
        }

        // Add the last argument
        size_t firstNonSpace = current.find_first_not_of(" \t");
        size_t lastNonSpace = current.find_last_not_of(" \t");
        if (firstNonSpace != std::string::npos) {
            typeArgs.push_back(current.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1));
        }

        return typeArgs;
    }

    void ClassRegistrar::parseAndStoreTypeSubstitutions(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> childClass,
        const std::string& parentClassNameWithGenerics,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentClass
    ) const
    {
        // Parse concrete type arguments from parent class declaration
        // E.g., "Processor<String>" → ["String"]
        auto typeArgs = parseGenericTypeArguments(parentClassNameWithGenerics);

        // Get generic type parameters from parent class definition
        // E.g., Processor<T> → ["T"]
        const auto& parentGenericParams = parentClass->getGenericParameters();

        // Create substitution map: generic param → concrete type
        // E.g., T → String
        std::unordered_map<std::string, std::string> substitutionMap;

        size_t numParams = std::min(typeArgs.size(), parentGenericParams.size());
        for (size_t i = 0; i < numParams; ++i) {
            substitutionMap[parentGenericParams[i].name] = typeArgs[i];
        }

        // Store in child class
        childClass->setParentTypeSubstitutionMap(substitutionMap);
    }

    void ClassRegistrar::parseAndStoreUnifiedTypeSubstitutions(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> childClass,
        const std::string& parentClassNameWithGenerics,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> parentClass
    ) const
    {
        // Parse concrete type arguments from parent class declaration
        // E.g., "Processor<String>" → ["String"]
        auto typeArgsStrings = parseGenericTypeArguments(parentClassNameWithGenerics);

        // Convert string type args to UnifiedTypePtr
        std::vector<::types::UnifiedTypePtr> typeArgs;
        for (const auto& argStr : typeArgsStrings) {
            // Simple conversion - could be enhanced for nested generics
            typeArgs.push_back(::types::UnifiedType::classType(argStr));
        }

        // Get generic type parameters from parent class definition
        const auto& parentGenericParams = parentClass->getGenericParameters();

        // Use TypeSubstitutionService to build the substitution map
        if (typeSubstitutionService) {
            try {
                auto unifiedSubstitutionMap = typeSubstitutionService->buildSubstitutionMap(
                    parentGenericParams,
                    typeArgs
                );

                // Convert UnifiedType substitution map to string-based map for backward compatibility
                std::unordered_map<std::string, std::string> stringSubstitutionMap;
                for (const auto& [paramName, typePtr] : unifiedSubstitutionMap) {
                    if (typePtr) {
                        stringSubstitutionMap[paramName] = typePtr->toString();
                    }
                }

                // Store in child class
                childClass->setParentTypeSubstitutionMap(stringSubstitutionMap);
            } catch (const std::exception&) {
                // Fall back to existing implementation if new service fails
                parseAndStoreTypeSubstitutions(childClass, parentClassNameWithGenerics, parentClass);
            }
        } else {
            // Fall back to existing implementation
            parseAndStoreTypeSubstitutions(childClass, parentClassNameWithGenerics, parentClass);
        }
    }
}
