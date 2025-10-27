#include "ClassRegistrar.hpp"
#include "../validation/CompileTimeValidator.hpp"
#include "../../../ast/nodes/statements/ProgramNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/ImportNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/InheritanceException.hpp"
#include "../../../errors/AbstractClassException.hpp"
#include "../../runtime/utils/TypeConverter.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../../../runtimeTypes/klass/FieldDefinition.hpp"
#include "../../../evaluator/validation/AnnotationValidator.hpp"
#include <stdexcept>

namespace vm::compiler::registration
{
    ClassRegistrar::ClassRegistrar(
        std::shared_ptr<environment::Environment> environment,
        bytecode::BytecodeProgram& program,
        InterfaceRegistrar* interfaceRegistrar
    )
        : environment(environment)
        , program(program)
        , interfaceRegistrar(interfaceRegistrar)
        , inheritanceValidator(std::make_unique<ClassInheritanceValidator>(environment->getClassRegistry()))
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

        // Copy annotations from AST to runtime definition
        for (const auto& annotation : classNode->getAnnotations()) {
            classDef->addAnnotation(annotation);
        }

        // Handle parent class
        if (classNode->hasParentClass()) {
            const std::string& parentClassName = classNode->getParentClassName();

            // Note: Parser already validates that classes cannot extend interfaces
            // at parse time (ClassDeclarationParser.cpp:92-98)

            classDef->setParentClassName(parentClassName);
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
                // Convert generic parameters to ValueType for constructor
                std::vector<std::pair<std::string, value::ValueType>> legacyParams;
                for (const auto& [name, genericType] : methodNode->getGenericParameters()) {
                    value::ValueType vType = value::ValueType::VOID;
                    if (genericType) {
                        vType = genericType->isGenericParameter() ? value::ValueType::OBJECT : genericType->getConcreteType();
                    }
                    legacyParams.emplace_back(name, vType);
                }

                auto methodDef = std::make_shared<runtimeTypes::klass::MethodDefinition>(
                    methodNode->getName(),
                    methodNode->getReturnType(),
                    legacyParams,
                    methodNode->getBody(),
                    methodNode->getIsStatic(),
                    methodNode->getAccessModifier()
                );

                // Set source location for error reporting
                methodDef->setSourceLocation(methodNode->getLocation());

                // Set generic type information for proper interface validation
                if (methodNode->getGenericReturnType()) {
                    methodDef->setGenericReturnType(methodNode->getGenericReturnType());
                }
                methodDef->setGenericParameters(methodNode->getGenericParameters());
                methodDef->setGenericTypeParameters(methodNode->getGenericTypeParameters());

                // Set abstract flag
                methodDef->setAbstract(methodNode->isAbstract());
                if (methodNode->isAbstract()) {
                    classDef->addAbstractMethod(methodNode->getName());
                }

                // Copy annotations from AST to runtime definition
                for (const auto& annotation : methodNode->getAnnotations()) {
                    methodDef->addAnnotation(annotation);
                }

                if (methodNode->getIsStatic()) {
                    classDef->addStaticMethod(methodNode->getName(), methodDef);
                } else {
                    classDef->addMethod(methodDef);
                }
            }
        }
        // Register fields (but don't initialize them - bytecode will do that)
        for (const auto& field : classNode->getFields()) {
            if (auto* fieldNode = dynamic_cast<ast::nodes::classes::FieldNode*>(field.get())) {
                auto fieldDef = std::make_shared<runtimeTypes::klass::FieldDefinition>(
                    fieldNode->getName(),
                    fieldNode->getType(),
                    std::monostate{},  // Empty value - bytecode will initialize
                    fieldNode->getIsStatic(),
                    fieldNode->getIsFinal(),
                    fieldNode->getAccessModifier()
                );

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

        // Validate abstract method implementations for classes without parents
        if (!classNode->hasParentClass()) {
            auto classDef = classRegistry->findClass(className);
            if (classDef && !classDef->isAbstract()) {
                // Concrete classes must not have abstract methods
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

            // Validate annotations for classes without parents
            // (e.g., @Override should fail if there's no parent)
            if (classDef) {
                evaluator::validation::AnnotationValidator::validateClassAnnotations(classDef, environment);
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

            // Validate inheritance depth after establishing the link
            validateInheritanceDepth(className, classNode->getLocation());

            // Validate annotations (e.g., @Override) after parent link is established
            evaluator::validation::AnnotationValidator::validateClassAnnotations(classDef, environment);

            // Validate method overrides
            validateMethodOverrides(classDef, parentDef, classNode);

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

        // Extract fields
        const auto& fields = classNode->getFields();
        for (const auto& fieldNode : fields) {
            auto* field = dynamic_cast<ast::FieldNode*>(fieldNode.get());
            if (!field) continue;

            bytecode::BytecodeProgram::FieldMetadata fieldMeta;
            fieldMeta.name = field->getName();
            fieldMeta.type = vm::runtime::utils::TypeConverter::valueTypeToString(field->getType());
            fieldMeta.isStatic = field->getIsStatic();
            fieldMeta.isFinal = field->getIsFinal();

            // Get access modifiers
            auto accessMod = field->getAccessModifier();
            fieldMeta.isPrivate = (accessMod == ast::AccessModifier::PRIVATE);
            fieldMeta.isProtected = (accessMod == ast::AccessModifier::PROTECTED);

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
            methodMeta.returnType = vm::runtime::utils::TypeConverter::valueTypeToString(method->getReturnType());
            methodMeta.isStatic = method->getIsStatic();
            methodMeta.isFinal = false;  // Methods don't currently support final modifier

            // Get access modifiers
            auto accessMod = method->getAccessModifier();
            methodMeta.isPrivate = (accessMod == ast::AccessModifier::PRIVATE);
            methodMeta.isProtected = (accessMod == ast::AccessModifier::PROTECTED);
            methodMeta.isAbstract = method->isAbstract();
            methodMeta.startOffset = 0;  // Will be set during bytecode generation if needed

            // Extract parameter types and names
            const auto& genericParams = method->getGenericParameters();
            for (const auto& [paramName, genericType] : genericParams) {
                value::ValueType vType = value::ValueType::VOID;
                if (genericType) {
                    vType = genericType->isGenericParameter() ? value::ValueType::OBJECT : genericType->getConcreteType();
                }
                methodMeta.parameterTypes.push_back(vm::runtime::utils::TypeConverter::valueTypeToString(vType));
                methodMeta.parameterNames.push_back(paramName);
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
        const auto& parentMethods = parentClass->getInstanceMethods();

        for (const auto& [methodName, childMethod] : childMethods) {
            auto parentIt = parentMethods.find(methodName);
            if (parentIt != parentMethods.end()) {
                // Method exists in parent - validate override
                auto parentMethod = parentIt->second;

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
                for (size_t i = 0; i < childParams.size(); ++i) {
                    const auto& childParam = childParams[i].second;
                    const auto& parentParam = parentParams[i].second;

                    // Compare using ParameterType equality operator
                    if (!(childParam == parentParam)) {
                        throw errors::InheritanceException(
                            "Method override parameter type mismatch in class '" + childClass->getName() +
                            "': method '" + methodName + "' parameter " + std::to_string(i + 1) +
                            " has type '" + childParam.toString() +
                            "' but parent method expects '" + parentParam.toString() + "'",
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
                        vm::runtime::utils::TypeConverter::valueTypeToString(childMethod->getReturnType()) +
                        "' but parent method has return type '" +
                        vm::runtime::utils::TypeConverter::valueTypeToString(parentMethod->getReturnType()) + "'",
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
