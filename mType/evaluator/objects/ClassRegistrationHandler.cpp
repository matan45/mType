#include "ClassRegistrationHandler.hpp"
#include "../interfaces/IExpressionEvaluator.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../ast/nodes/classes/InterfaceNode.hpp"
#include "../../ast/nodes/classes/FieldNode.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../runtimeTypes/klass/FieldDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../../errors/TypeException.hpp"
#include "../validation/InheritanceValidator.hpp"
#include "../utils/ValueConverter.hpp"
#include "../../value/ParameterType.hpp"
#include "../../circularDependency/CircularDependencyDetector.hpp"
#include <sstream>

using namespace errors;
using evaluator::utils::ValueConverter;

namespace evaluator
{
    namespace objects
    {
        ClassRegistrationHandler::ClassRegistrationHandler(std::shared_ptr<EvaluationContext> ctx)
            : context(ctx), exprEvaluator(nullptr)
        {
        }

        void ClassRegistrationHandler::setExpressionEvaluator(interfaces::IExpressionEvaluator* evaluator)
        {
            exprEvaluator = evaluator;
        }

        Value ClassRegistrationHandler::evaluateClass(ClassNode* node)
        {
            auto env = context->getEnvironment();

            // Create class definition with generic parameters if present
            const auto& genericParams = node->getGenericParameters();

            auto classDef = std::make_shared<ClassDefinition>(node->getClassName(), node->getGenericParameters());

            // Set final modifier
            classDef->setFinal(node->isFinal());

            // Set implemented interfaces
            classDef->setImplementedInterfaces(node->getImplementedInterfaces());

            // NEW: Handle inheritance if parent class specified
            if (node->hasParentClass()) {
                const std::string& parentClassName = node->getParentClassName();

                // Validate parent class exists
                validation::InheritanceValidator::validateParentClassExists(
                    parentClassName,
                    node->getLocation(),
                    context);

                // Validate no circular inheritance
                static circularDependency::CircularDependencyDetector inheritanceDetector;
                validation::InheritanceValidator::validateCircularInheritance(
                    node->getClassName(),
                    parentClassName,
                    node->getLocation(),
                    inheritanceDetector);

                // Set parent class name in definition
                classDef->setParentClassName(parentClassName);

                // Extract base class name from generic type (e.g., "Container<T>" -> "Container")
                std::string baseParentClassName = parentClassName;
                size_t genericStart = parentClassName.find('<');
                if (genericStart != std::string::npos) {
                    baseParentClassName = parentClassName.substr(0, genericStart);
                }

                // Find parent class and link it (use base class name for lookup)
                auto parentClass = env->findClass(baseParentClassName);
                if (parentClass) {
                    classDef->setParentClass(parentClass);

                    // Register inheritance relationship in class registry (use base name)
                    env->getClassRegistry()->registerInheritance(
                        node->getClassName(),
                        baseParentClassName);

                    // Validate inheritance depth
                    validation::InheritanceValidator::validateInheritanceDepth(
                        node->getClassName(),
                        node->getLocation(),
                        context);
                }
            }

            // Add fields
            for (const auto& fieldPtr : node->getFields())
            {
                auto fieldNode = dynamic_cast<FieldNode*>(fieldPtr.get());
                if (!fieldNode) continue;

                // Evaluate initial value if present
                Value initialValue{};
                if (fieldNode->hasInitialValue() && exprEvaluator)
                {
                    initialValue = exprEvaluator->evaluate(fieldNode->getInitialValue());
                }

                auto fieldDef = std::make_shared<FieldDefinition>(
                    fieldNode->getName(),
                    fieldNode->getType(),
                    initialValue,
                    fieldNode->getIsStatic(),
                    fieldNode->getIsFinal(),
                    fieldNode->getAccessModifier() // Pass access modifier from AST
                );
                classDef->addField(fieldDef);
            }

            // Add methods
            for (const auto& methodPtr : node->getMethods())
            {
                auto methodNode = dynamic_cast<MethodNode*>(methodPtr.get());
                if (!methodNode) continue;

                // Get shared_ptr to method body safely
                auto bodyPtr = methodNode->getBody();

                // Create method definition with generic type information preserved
                std::shared_ptr<MethodDefinition> methodDef;

                // Convert MethodNode parameters to ParameterType for interface support
                auto convertToParameterTypes = [this, env, methodNode]() -> std::vector<std::pair<std::string, ParameterType>>
                {
                    std::vector<std::pair<std::string, ParameterType>> newParams;

                    // Get generic parameters which contain the actual type names (including interfaces)
                    auto genericParams = methodNode->getGenericParameters();
                    auto legacyParams = methodNode->getParameters();

                    for (size_t i = 0; i < legacyParams.size(); ++i)
                    {
                        const std::string& paramName = legacyParams[i].first;
                        ValueType baseType = legacyParams[i].second;

                        // Single construction using helper function for efficiency and clarity
                        std::shared_ptr<ast::GenericType> genericType =
                            (i < genericParams.size()) ? genericParams[i].second : nullptr;

                        const ParameterType paramType = createParameterType(baseType, genericType, env.get());

                        newParams.emplace_back(paramName, paramType);
                    }

                    return newParams;
                };

                auto parameterTypes = convertToParameterTypes();

                if (methodNode->isGeneric())
                {
                    // For generic methods, preserve the generic type information
                    methodDef = std::make_shared<MethodDefinition>(
                        methodNode->getName(),
                        methodNode->getReturnType(), // Legacy ValueType for compatibility
                        parameterTypes, // NEW: Use ParameterType instead of ValueType
                        std::vector<std::pair<std::string, Value>>{}, // empty arguments
                        bodyPtr,
                        methodNode->getIsStatic(),
                        methodNode->getGenericReturnType(), // NEW: Preserve generic return type
                        methodNode->getGenericParameters(), // NEW: Preserve generic method parameters
                        methodNode->getGenericTypeParameters(), // NEW: Preserve generic type parameter declarations
                        std::unordered_map<std::string, std::string>{}, // Empty substitution map for template
                        methodNode->getAccessModifier() // Pass access modifier from AST
                    );
                }
                else
                {
                    // For non-generic methods, also preserve type information for object parameters
                    methodDef = std::make_shared<MethodDefinition>(
                        methodNode->getName(),
                        methodNode->getReturnType(), // Legacy ValueType for compatibility
                        parameterTypes, // NEW: Use ParameterType instead of ValueType
                        std::vector<std::pair<std::string, Value>>{}, // empty arguments
                        bodyPtr,
                        methodNode->getIsStatic(),
                        methodNode->getGenericReturnType(), // NEW: Preserve return type for object types
                        methodNode->getGenericParameters(), // NEW: Preserve parameter types for object types
                        std::vector<GenericTypeParameter>{}, // No generic type parameters for non-generic methods
                        std::unordered_map<std::string, std::string>{}, // Empty substitution map
                        methodNode->getAccessModifier() // Pass access modifier from AST
                    );
                }

                // NEW: Copy async flag from AST to runtime definition
                methodDef->setIsAsync(methodNode->getIsAsync());

                classDef->addMethod(methodDef);
            }

            // Add constructors
            for (const auto& constructorPtr : node->getConstructors())
            {
                auto constructorNode = dynamic_cast<ConstructorNode*>(constructorPtr.get());
                if (!constructorNode) continue;

                // Get shared_ptr to constructor body safely
                auto bodyPtr = constructorNode->getBody();

                // Create constructor definition with full type information
                auto ctorDef = std::make_shared<ConstructorDefinition>(
                    constructorNode->getParametersWithTypes(),
                    bodyPtr,
                    constructorNode->getAccessModifier() // Pass access modifier from AST
                );

                // Copy super initializer if present
                if (constructorNode->hasSuperInitializer()) {
                    auto superInit = constructorNode->getSuperInitializer();
                    // Convert raw pointer to shared_ptr
                    ctorDef->setSuperInitializer(std::shared_ptr<::ast::nodes::classes::SuperConstructorCallNode>(
                        superInit, [](auto*){})); // Empty deleter since we don't own the original
                }

                classDef->addConstructor(ctorDef);
            }

            // Validate interface implementations
            validateInterfaceImplementations(classDef);

            // NEW: Validate method overrides if class has parent
            if (classDef->hasParentClass()) {
                auto parentClass = classDef->getParentClass();
                if (parentClass) {
                    validation::InheritanceValidator::validateMethodOverrides(
                        classDef,
                        parentClass,
                        node->getLocation());
                }
            }

            // Register class
            registerClass(classDef);

            return std::monostate{};
        }

        Value ClassRegistrationHandler::evaluateInterface(InterfaceNode* node)
        {
            auto env = context->getEnvironment();

            // Create interface definition with generic parameters if present
            const auto& genericParams = node->getGenericParameters();
            const auto& extendsInterfaces = node->getExtendedInterfaces();

            // No runtime validation needed - all checks done at parse time

            auto interfaceDef = std::make_shared<InterfaceDefinition>(
                node->getName(), genericParams, extendsInterfaces);

            // Set final modifier
            interfaceDef->setFinal(node->isFinal());

            // Process method signatures
            for (const auto& methodPtr : node->getMethods())
            {
                auto functionNode = dynamic_cast<ast::nodes::functions::FunctionNode*>(methodPtr.get());
                if (!functionNode) continue;

                // Create method signature from function node
                MethodSignature signature;
                signature.name = functionNode->getName();
                signature.returnType = functionNode->getGenericReturnType();

                // Add parameters
                const auto& parameters = functionNode->getGenericParameters();
                for (const auto& param : parameters)
                {
                    signature.parameters.emplace_back(param.first, param.second);
                }

                // Add generic parameters if method is generic
                if (functionNode->isGeneric())
                {
                    signature.genericParameters = functionNode->getGenericTypeParameters();
                }

                interfaceDef->addMethodSignature(signature);
            }

            // Register interface first (needed for hierarchy validation)
            env->registerInterface(node->getName(), interfaceDef);

            // Validate interface hierarchy (prevent circular inheritance)
            if (!env->getInterfaceRegistry()->validateInterfaceHierarchy(node->getName()))
            {
                // Unregister the interface since validation failed
                env->getInterfaceRegistry()->removeInterface(node->getName());
                throw circularDependency::CircularDependencyException(
                    "Circular interface inheritance detected for interface '" + node->getName() + "'",
                    {node->getName()}, // dependency chain
                    node->getLocation().toString()
                );
            }

            return std::monostate{};
        }

        void ClassRegistrationHandler::registerClass(std::shared_ptr<ClassDefinition> classDef)
        {
            auto env = context->getEnvironment();
            env->registerClass(classDef->getName(), classDef);
        }

        ParameterType ClassRegistrationHandler::createParameterType(
            ValueType baseType,
            std::shared_ptr<ast::GenericType> genericType,
            environment::Environment* env) const
        {
            // If no generic information available, use basic type
            if (!genericType) {
                return ParameterType(baseType);
            }

            // Case 1: Generic parameter (might be interface/class or generic type variable)
            if (genericType->isGenericParameter()) {
                std::string typeName = genericType->getGenericName();

                // Check if it's a known interface first
                if (env->findInterface(typeName) != nullptr) {
                    return ParameterType::forInterface(typeName);
                }
                // Check if it's a known class
                if (env->findClass(typeName) != nullptr) {
                    return ParameterType::forClass(typeName);
                }
                // Otherwise, it's a generic type variable (T, E, etc.)
                return ParameterType(baseType);
            }

            // Case 2: Object type with explicit type name
            if (baseType == ValueType::OBJECT) {
                std::string typeName = genericType->getBaseTypeName();

                // Check if it's a registered interface
                if (env->findInterface(typeName) != nullptr) {
                    return ParameterType::forInterface(typeName);
                }
                // Check if it's a registered class
                if (env->findClass(typeName) != nullptr) {
                    return ParameterType::forClass(typeName);
                }
            }

            // Default: use basic type
            return ParameterType(baseType);
        }

        void ClassRegistrationHandler::validateInterfaceImplementations(std::shared_ptr<ClassDefinition> classDef)
        {
            auto env = context->getEnvironment();
            auto interfaceRegistry = env->getInterfaceRegistry();

            for (const auto& interfaceName : classDef->getImplementedInterfaces())
            {
                // Parse interface name and extract generic type arguments
                auto [baseInterfaceName, typeArguments] = parseGenericInterfaceName(interfaceName);

                // Get interface definition
                auto interfaceDef = interfaceRegistry->findInterface(baseInterfaceName);
                if (!interfaceDef)
                {
                    throw TypeException("Interface '" + baseInterfaceName + "' not found");
                }

                const auto& interfaceGenericParams = interfaceDef->getGenericParameters();

                // Create type substitution map for generic resolution
                std::unordered_map<std::string, std::string> typeSubstitutions;

                if (typeArguments.size() != interfaceGenericParams.size())
                {
                    throw TypeException("Interface '" + baseInterfaceName + "' expects " +
                        std::to_string(interfaceGenericParams.size()) + " type arguments but got " +
                        std::to_string(typeArguments.size()));
                }

                // Map generic parameters to concrete types
                for (size_t i = 0; i < interfaceGenericParams.size(); ++i)
                {
                    typeSubstitutions[interfaceGenericParams[i].name] = typeArguments[i];
                }

                // Check that all interface methods are implemented
                const auto& methodSignatures = interfaceDef->getMethodSignatures();
                for (const auto& signature : methodSignatures)
                {
                    auto method = classDef->getMethod(signature.name);
                    if (!method)
                    {
                        throw TypeException("Class '" + classDef->getName() +
                            "' must implement method '" + signature.name +
                            "' from interface '" + interfaceName + "'");
                    }

                    // Resolve return type with substitutions
                    std::string resolvedReturnType = resolveGenericType(signature.returnType->getBaseTypeName(),
                                                                        typeSubstitutions);
                    std::string methodReturnType;

                    // Use generic return type if available, otherwise fall back to basic ValueType
                    if (method->getGenericReturnType())
                    {
                        methodReturnType = method->getGenericReturnType()->getBaseTypeName();
                    }
                    else
                    {
                        methodReturnType = ValueConverter::valueTypeToString(method->getReturnType());
                    }

                    if (methodReturnType != resolvedReturnType)
                    {
                        throw TypeException("Method '" + signature.name + "' in class '" + classDef->getName() +
                            "' has return type '" + methodReturnType +
                            "' but interface '" + interfaceName + "' requires '" +
                            resolvedReturnType + "'");
                    }

                    // Validate parameter count matches
                    if (method->getParameters().size() != signature.parameters.size())
                    {
                        throw TypeException("Method '" + signature.name + "' in class '" + classDef->getName() +
                            "' has " + std::to_string(method->getParameters().size()) + " parameters" +
                            " but interface '" + interfaceName + "' requires " +
                            std::to_string(signature.parameters.size()) + " parameters");
                    }

                    // Validate parameter types with generic resolution
                    const auto& methodParams = method->getParameters();
                    const auto& methodGenericParams = method->getGenericParameters();
                    for (size_t i = 0; i < signature.parameters.size(); ++i)
                    {
                        std::string methodParamType;

                        // Use generic parameter type if available, otherwise fall back to basic ValueType
                        if (i < methodGenericParams.size() && methodGenericParams[i].second)
                        {
                            methodParamType = methodGenericParams[i].second->getBaseTypeName();
                        }
                        else
                        {
                            methodParamType = ValueConverter::valueTypeToString(methodParams[i].second);
                        }

                        std::string resolvedParamType = resolveGenericType(
                            signature.parameters[i].second->getBaseTypeName(), typeSubstitutions);

                        if (methodParamType != resolvedParamType)
                        {
                            throw TypeException("Method '" + signature.name + "' parameter " + std::to_string(i + 1) +
                                " in class '" + classDef->getName() + "' has type '" +
                                methodParamType + "' but interface '" + interfaceName +
                                "' requires '" + resolvedParamType + "'");
                        }
                    }
                }
            }
        }

        std::pair<std::string, std::vector<std::string>> ClassRegistrationHandler::parseGenericInterfaceName(
            const std::string& interfaceName)
        {
            // Check if this is a generic interface (contains '<' and '>')
            size_t openBracket = interfaceName.find('<');
            if (openBracket == std::string::npos)
            {
                // Non-generic interface
                return {interfaceName, {}};
            }

            size_t closeBracket = interfaceName.find('>', openBracket);
            if (closeBracket == std::string::npos)
            {
                throw TypeException("Malformed generic interface name: " + interfaceName);
            }

            std::string baseInterfaceName = interfaceName.substr(0, openBracket);
            std::string typeArgsStr = interfaceName.substr(openBracket + 1, closeBracket - openBracket - 1);

            std::vector<std::string> typeArguments;

            // Parse type arguments (simple comma-separated parsing)
            if (!typeArgsStr.empty())
            {
                std::stringstream ss(typeArgsStr);
                std::string typeArg;
                while (std::getline(ss, typeArg, ','))
                {
                    // Trim whitespace
                    typeArg.erase(0, typeArg.find_first_not_of(" \t"));
                    typeArg.erase(typeArg.find_last_not_of(" \t") + 1);
                    if (!typeArg.empty())
                    {
                        typeArguments.push_back(typeArg);
                    }
                }
            }

            return {baseInterfaceName, typeArguments};
        }

        std::string ClassRegistrationHandler::resolveGenericType(const std::string& typeName,
                                                                 const std::unordered_map<std::string, std::string>&
                                                                 typeSubstitutions)
        {
            // Check if this type needs to be substituted
            auto it = typeSubstitutions.find(typeName);
            if (it != typeSubstitutions.end())
            {
                return it->second;
            }

            // If no substitution found, return the original type name
            return typeName;
        }
    }
}
