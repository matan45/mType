#include "ClassRegistrar.hpp"
#include "../../../ast/nodes/statements/ProgramNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/ImportNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/InheritanceException.hpp"
#include "../../../evaluator/utils/ValueConverter.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../../../runtimeTypes/klass/FieldDefinition.hpp"
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

        // Handle parent class
        if (classNode->hasParentClass()) {
            const std::string& parentClassName = classNode->getParentClassName();

            // Validate that class is not trying to extend an interface
            // Extract base parent name from generic type
            std::string baseParentName = parentClassName;
            size_t genericStart = parentClassName.find('<');
            if (genericStart != std::string::npos) {
                baseParentName = parentClassName.substr(0, genericStart);
            }

            // Check if the parent is actually an interface
            auto interfaceRegistry = environment->getInterfaceRegistry();
            if (interfaceRegistry && interfaceRegistry->hasInterface(baseParentName)) {
                throw errors::InheritanceException(
                    "Class '" + className + "' cannot extend interface '" + parentClassName + "'. "
                    "Classes can only extend other classes. Use 'implements' to implement interfaces.",
                    className,
                    parentClassName,
                    classNode->getLocation());
            }

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
                auto methodDef = std::make_shared<runtimeTypes::klass::MethodDefinition>(
                    methodNode->getName(),
                    methodNode->getReturnType(),
                    methodNode->getParameters(),
                    std::vector<std::pair<std::string, value::Value>>{},
                    methodNode->getBody(),
                    methodNode->getIsStatic(),
                    methodNode->getAccessModifier()
                );

                // Set generic type information for proper interface validation
                if (methodNode->getGenericReturnType()) {
                    methodDef->setGenericReturnType(methodNode->getGenericReturnType());
                }
                methodDef->setGenericParameters(methodNode->getGenericParameters());
                methodDef->setGenericTypeParameters(methodNode->getGenericTypeParameters());

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
            interfaceRegistrar->validateInterfaceImplementations(classDef, classNode->getLocation());
        }

        // Extract and store class metadata for bytecode serialization
        auto classMetadata = extractClassMetadata(classNode);
        program.registerClass(classMetadata);
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
        if (!classNode->hasParentClass()) {
            return;
        }

        std::string className = classNode->getClassName();
        std::string parentClassName = classNode->getParentClassName();

        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            throw std::runtime_error("Class registry not available");
        }

        // Strip generic type parameters from parent class name: "Container<T>" -> "Container"
        size_t genericStart = parentClassName.find('<');
        std::string baseParentClassName = (genericStart != std::string::npos)
            ? parentClassName.substr(0, genericStart)
            : parentClassName;

        // Get both class definitions
        auto classDef = classRegistry->findClass(className);
        auto parentDef = classRegistry->findClass(baseParentClassName);

        if (classDef && parentDef) {
            // Check for circular inheritance before establishing the link
            checkCircularInheritance(className, parentClassName, parentDef);

            // Establish the parent-child link
            classDef->setParentClass(parentDef);
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
            fieldMeta.type = evaluator::utils::ValueConverter::valueTypeToString(field->getType());
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
            methodMeta.returnType = evaluator::utils::ValueConverter::valueTypeToString(method->getReturnType());
            methodMeta.isStatic = method->getIsStatic();
            methodMeta.isFinal = false;  // Methods don't currently support final modifier

            // Get access modifiers
            auto accessMod = method->getAccessModifier();
            methodMeta.isPrivate = (accessMod == ast::AccessModifier::PRIVATE);
            methodMeta.isProtected = (accessMod == ast::AccessModifier::PROTECTED);
            methodMeta.startOffset = 0;  // Will be set during bytecode generation if needed

            // Extract parameter types and names
            const auto& params = method->getParameters();
            for (const auto& param : params) {
                methodMeta.parameterTypes.push_back(evaluator::utils::ValueConverter::valueTypeToString(param.second));
                methodMeta.parameterNames.push_back(param.first);
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
}
