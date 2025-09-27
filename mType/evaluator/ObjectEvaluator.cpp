#include "ObjectEvaluator.hpp"
#include "../constants/LambdaConstants.hpp"
#include "../value/LambdaValue.hpp"
#include "../value/ParameterType.hpp"
#include "utils/ParameterBinder.hpp"
#include "utils/ScopeGuard.hpp"
#include "utils/GenericTypeManager.hpp"
#include <mutex>
#include <iostream>
#include "../value/FlatMultiArray.hpp"
#include "../value/SparseMultiArray.hpp"
#include "../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/UndefinedException.hpp"
#include "../exception/ReturnException.hpp"
#include "../environment/manager/Scope.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../ast/nodes/classes/FieldNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../ast/nodes/classes/ConstructorNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/classes/InterfaceNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"
#include "../ast/nodes/classes/NewNode.hpp"
#include "../ast/nodes/classes/MemberAccessNode.hpp"
#include "../ast/nodes/expressions/VariableNode.hpp"
#include "../ast/nodes/classes/MethodCallNode.hpp"
#include "../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../ast/nodes/statements/IndexAssignmentNode.hpp"
#include "../value/NativeArray.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/MethodDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../parser/TypeParser.hpp"
#include "../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "ExpressionEvaluator.hpp"
#include "StatementEvaluator.hpp"

namespace evaluator
{
    using namespace errors;
    using namespace runtimeTypes::klass;
    using namespace parser;

    ObjectEvaluator::ObjectEvaluator(std::shared_ptr<EvaluationContext> ctx)
        : context(ctx), instanceManager(std::make_unique<InstanceManager>()),
          exprEvaluator(nullptr), stmtEvaluator(nullptr)
    {
    }

    Value ObjectEvaluator::evaluate(ASTNode* node)
    {
        if (!node || !canHandle(node))
        {
            return std::monostate{};
        }

        // Dispatch to appropriate evaluation method based on node type
        if (auto classNode = dynamic_cast<ClassNode*>(node))
        {
            return evaluateClassNode(classNode);
        }
        if (auto interfaceNode = dynamic_cast<InterfaceNode*>(node))
        {
            return evaluateInterfaceNode(interfaceNode);
        }
        if (auto methodNode = dynamic_cast<MethodNode*>(node))
        {
            return evaluateMethodNode(methodNode);
        }
        if (auto fieldNode = dynamic_cast<FieldNode*>(node))
        {
            return evaluateFieldNode(fieldNode);
        }
        if (auto ctorNode = dynamic_cast<ConstructorNode*>(node))
        {
            return evaluateConstructorNode(ctorNode);
        }
        if (auto newNode = dynamic_cast<NewNode*>(node))
        {
            return evaluateNewNode(newNode);
        }
        if (auto memberAccessNode = dynamic_cast<MemberAccessNode*>(node))
        {
            return evaluateMemberAccessNode(memberAccessNode);
        }
        if (auto methodCallNode = dynamic_cast<MethodCallNode*>(node))
        {
            return evaluateMethodCallNode(methodCallNode);
        }
        if (auto memberAssignNode = dynamic_cast<MemberAssignmentNode*>(node))
        {
            return evaluateMemberAssignmentNode(memberAssignNode);
        }
        if (auto indexAssignNode = dynamic_cast<IndexAssignmentNode*>(node))
        {
            return evaluateIndexAssignmentNode(indexAssignNode);
        }

        return std::monostate{};
    }

    bool ObjectEvaluator::canHandle(ASTNode* node) const
    {
        return isObjectNode(node);
    }


    void ObjectEvaluator::setExpressionEvaluator(ExpressionEvaluator* evaluator)
    {
        exprEvaluator = evaluator;
    }

    void ObjectEvaluator::setStatementEvaluator(StatementEvaluator* evaluator)
    {
        stmtEvaluator = evaluator;
    }

    bool ObjectEvaluator::isObjectNode(ASTNode* node) const
    {
        return dynamic_cast<ClassNode*>(node) ||
            dynamic_cast<InterfaceNode*>(node) ||
            dynamic_cast<MethodNode*>(node) ||
            dynamic_cast<FieldNode*>(node) ||
            dynamic_cast<ConstructorNode*>(node) ||
            dynamic_cast<NewNode*>(node) ||
            dynamic_cast<MemberAccessNode*>(node) ||
            dynamic_cast<MethodCallNode*>(node) ||
            dynamic_cast<MemberAssignmentNode*>(node) ||
            dynamic_cast<IndexAssignmentNode*>(node);
    }

    Value ObjectEvaluator::evaluateClassNode(ClassNode* node)
    {
        auto env = context->getEnvironment();

        // Create class definition with generic parameters if present
        const auto& genericParams = node->getGenericParameters();

        // Validate generic parameter count limit
        if (genericParams.size() > MAX_GENERIC_PARAMETERS) {
            throw mtype::exceptions::TypeException(
                "Class '" + node->getClassName() + "' has too many generic parameters (" +
                std::to_string(genericParams.size()) + "). Maximum allowed: " +
                std::to_string(MAX_GENERIC_PARAMETERS),
                node->getLocation().toString()
            );
        }

        auto classDef = std::make_shared<ClassDefinition>(node->getClassName(), node->getGenericParameters());

        // Set implemented interfaces
        classDef->setImplementedInterfaces(node->getImplementedInterfaces());

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
                fieldNode->getIsFinal()
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
            auto convertToParameterTypes = [env, methodNode]() -> std::vector<std::pair<std::string, ParameterType>> {
                std::vector<std::pair<std::string, ParameterType>> newParams;

                // Get generic parameters which contain the actual type names (including interfaces)
                auto genericParams = methodNode->getGenericParameters();
                auto legacyParams = methodNode->getParameters();

                for (size_t i = 0; i < legacyParams.size(); ++i) {
                    const std::string& paramName = legacyParams[i].first;
                    ValueType baseType = legacyParams[i].second;

                    ParameterType paramType(baseType);  // Initialize with base type

                    // If we have generic parameter information, extract interface name
                    if (i < genericParams.size() && genericParams[i].second) {
                        auto genericType = genericParams[i].second;
                        if (genericType->isGenericParameter()) {
                            // This might be a generic parameter OR an interface/class name
                            std::string typeName = genericType->getGenericName();

                            // Check if it's a known interface or class first
                            if (env->findInterface(typeName) != nullptr) {
                                paramType = ParameterType::forInterface(typeName);
                            }
                            else if (env->findClass(typeName) != nullptr) {
                                paramType = ParameterType::forClass(typeName);
                            }
                            else {
                                // This is actually a generic parameter like T, E, etc.
                                paramType = ParameterType(baseType);
                            }
                        } else if (baseType == ValueType::OBJECT) {
                            // This might be an interface or class - check environment to determine
                            std::string typeName = genericType->getBaseTypeName();

                            // Check if it's a registered interface
                            if (env->findInterface(typeName) != nullptr) {
                                paramType = ParameterType::forInterface(typeName);
                            }
                            // Check if it's a registered class
                            else if (env->findClass(typeName) != nullptr) {
                                paramType = ParameterType::forClass(typeName);
                            }
                            else {
                                // Unknown type - default to basic object type
                                paramType = ParameterType(baseType);
                            }
                        }
                    }

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
                    std::unordered_map<std::string, std::string>{} // Empty substitution map for template
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
                    std::vector<ast::GenericTypeParameter>{}, // No generic type parameters for non-generic methods
                    std::unordered_map<std::string, std::string>{} // Empty substitution map
                );
            }

            classDef->addMethod(methodDef);
        }

        // Add constructors
        for (const auto& constructorPtr : node->getConstructors())
        {
            auto constructorNode = dynamic_cast<ConstructorNode*>(constructorPtr.get());
            if (!constructorNode) continue;

            // Get shared_ptr to constructor body safely
            auto bodyPtr = constructorNode->getBody();

            auto ctorDef = std::make_shared<ConstructorDefinition>(
                constructorNode->getParameters(),
                bodyPtr
            );

            classDef->addConstructor(ctorDef);
        }

        // Validate interface implementations
        validateInterfaceImplementations(classDef, node);

        // Register class
        registerClass(classDef);

        return std::monostate{};
    }

    Value ObjectEvaluator::evaluateInterfaceNode(InterfaceNode* node)
    {
        auto env = context->getEnvironment();

        // Create interface definition with generic parameters if present
        const auto& genericParams = node->getGenericParameters();
        const auto& extendsInterfaces = node->getExtendedInterfaces();

        // Validate generic parameter count limit
        if (genericParams.size() > MAX_GENERIC_PARAMETERS) {
            throw mtype::exceptions::TypeException(
                "Interface '" + node->getName() + "' has too many generic parameters (" +
                std::to_string(genericParams.size()) + "). Maximum allowed: " +
                std::to_string(MAX_GENERIC_PARAMETERS),
                node->getLocation().toString()
            );
        }

        auto interfaceDef = std::make_shared<runtimeTypes::klass::InterfaceDefinition>(
            node->getName(), genericParams, extendsInterfaces);

        // Process method signatures
        for (const auto& methodPtr : node->getMethods())
        {
            auto functionNode = dynamic_cast<ast::nodes::functions::FunctionNode*>(methodPtr.get());
            if (!functionNode) continue;

            // Create method signature from function node
            runtimeTypes::klass::MethodSignature signature;
            signature.name = functionNode->getName();
            signature.returnType = functionNode->getGenericReturnType();

            // Add parameters
            const auto& parameters = functionNode->getGenericParameters();
            for (const auto& param : parameters) {
                signature.parameters.emplace_back(param.first, param.second);
            }

            // Add generic parameters if method is generic
            if (functionNode->isGeneric()) {
                signature.genericParameters = functionNode->getGenericTypeParameters();
            }

            interfaceDef->addMethodSignature(signature);
        }

        // Register interface first (needed for hierarchy validation)
        env->registerInterface(node->getName(), interfaceDef);

        // Validate interface hierarchy (prevent circular inheritance)
        if (!env->getInterfaceRegistry()->validateInterfaceHierarchy(node->getName())) {
            // Unregister the interface since validation failed
            env->getInterfaceRegistry()->removeInterface(node->getName());
            throw mtype::exceptions::CircularDependencyException(
                "Circular interface inheritance detected for interface '" + node->getName() + "'",
                {node->getName()}, // dependency chain
                node->getLocation().toString()
            );
        }

        return std::monostate{};
    }

    void ObjectEvaluator::registerClass(std::shared_ptr<ClassDefinition> classDef)
    {
        auto env = context->getEnvironment();
        env->registerClass(classDef->getName(), classDef);
    }

    Value ObjectEvaluator::evaluateNewNode(NewNode* node)
    {
        std::vector<Value> args;
        if (exprEvaluator)
        {
            args = evaluateArgumentList(node->getArguments());
        }

        std::string className = node->getClassName();


        // Handle regular class instantiation
        // className already declared above for collections
        std::shared_ptr<ClassDefinition> classDef;
        auto env = context->getEnvironment();

        // Try to resolve type parameters from current object context first
        std::string resolvedClassName = className;
        if (utils::GenericTypeManager::isGenericInstantiation(className))
        {
            auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(className);

            std::vector<std::string> resolvedTypeArguments;
            bool hasResolvedParams = false;
            for (const std::string& typeArg : typeArguments)
            {
                std::string resolvedType = resolveTypeParameterFromContext(typeArg);
                resolvedTypeArguments.push_back(resolvedType);
                if (resolvedType != typeArg)
                {
                    hasResolvedParams = true;
                }
            }

            // If we resolved any type parameters, construct the resolved class name
            if (hasResolvedParams)
            {
                resolvedClassName = baseName + "<" + resolvedTypeArguments[0];
                for (size_t i = 1; i < resolvedTypeArguments.size(); ++i)
                {
                    resolvedClassName += "," + resolvedTypeArguments[i];
                }
                resolvedClassName += ">";
            }
        }

        // Check if we should use the resolved class name for direct instantiation
        if (resolvedClassName != className)
        {
            // We resolved type parameters - try to find the instantiated class directly
            auto instantiatedClass = env->findClass(resolvedClassName);
            if (instantiatedClass)
            {
                classDef = instantiatedClass;
            }
            else
            {
                className = resolvedClassName; // Use resolved name for generic instantiation
            }
        }

        // Check if this is a generic instantiation (only if we haven't already found instantiated class)
        if (!classDef && utils::GenericTypeManager::isGenericInstantiation(className))
        {
            // Parse generic instantiation
            auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(className);

            // Find the generic class template
            auto genericClass = env->findClass(baseName);
            if (!genericClass)
            {
                throw UndefinedException("Generic class '" + baseName + "' not found");
            }

            if (!genericClass->isGeneric())
            {
                throw TypeException("Class '" + baseName + "' is not generic but used with type arguments",
                                   node->getLocation());
            }

            // Validate type arguments
            if (!utils::GenericTypeManager::validateTypeArguments(genericClass, typeArguments))
            {
                throw TypeException("Invalid type arguments for generic class '" + baseName + "'",
                                   node->getLocation());
            }

            // Create instantiated class
            classDef = utils::GenericTypeManager::instantiateGenericClass(genericClass, typeArguments);
            if (!classDef)
            {
                throw TypeException("Failed to instantiate generic class '" + className + "'",
                                   node->getLocation());
            }

            // Register the instantiated class for future use
            env->registerClass(className, classDef);
        }
        else
        {
            // Regular non-generic class - use resolved class name if available
            std::string classNameToLookup = (resolvedClassName != className) ? resolvedClassName : className;
            classDef = env->findClass(classNameToLookup);
        }

        if (!classDef)
        {
            std::string classNameToReport = (resolvedClassName != className) ? resolvedClassName : className;
            throw UndefinedException("Class '" + classNameToReport + "' not found");
        }

        std::string classNameForInstance = (resolvedClassName != className) ? resolvedClassName : className;

        // Extract generic type bindings for this instance
        std::unordered_map<std::string, std::string> genericTypeBindings;
        std::string baseClassName = classNameForInstance;

        if (utils::GenericTypeManager::isGenericInstantiation(classNameForInstance))
        {
            auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(classNameForInstance);

            // Debug logging
            for (size_t i = 0; i < typeArguments.size(); ++i)
            {
            }

            // Use the base class name for lookup (e.g., "LinkedList" instead of "LinkedList<String>")
            baseClassName = baseName;

            // Look up the generic template class
            auto templateClassDef = env->findClass(baseClassName);
            if (templateClassDef)
            {
            }

            if (templateClassDef && templateClassDef->isGeneric())
            {
                const auto& genericParams = templateClassDef->getGenericParameters();

                // Map each generic parameter to its concrete type
                for (size_t i = 0; i < genericParams.size() && i < typeArguments.size(); ++i)
                {
                    genericTypeBindings[genericParams[i].name] = typeArguments[i];
                }

                // Use the template class definition for creating the instance
                classDef = templateClassDef;
            }
        }

        auto instance = createInstanceWithTypeBindings(classNameForInstance, args, genericTypeBindings);

        // Debug: Print the created instance's type bindings
        if (!genericTypeBindings.empty())
        {
            for (const auto& binding : genericTypeBindings)
            {
            }
        }

        // Execute constructor if it exists
        if (classDef && !classDef->getConstructors().empty())
        {
            auto constructor = classDef->findConstructor(args.size());
            if (!constructor)
            {
                // Class has constructors but none match the provided arguments
                throw TypeException("No matching constructor for class '" + node->getClassName() +
                                    "' with " + std::to_string(args.size()) + " argument(s)", node->getLocation());
            }
            if (constructor && constructor->getBody())
            {
                // Set the current instance for constructor execution
                auto prevInstance = context->getCurrentInstance();
                context->setCurrentInstance(instance);

                // Temporarily clear static method context for constructor execution
                // Constructors always run in instance context regardless of where they're called from
                bool wasInStaticMethod = context->isInStaticMethodContext();
                context->setInStaticMethod(false);

                // Use ScopeGuard for automatic scope management
                {
                    utils::ScopeGuard scope(env, "constructor", environment::manager::ScopeType::FUNCTION);

                    try
                    {
                        // Use ParameterBinder utility to eliminate duplication
                        utils::ParameterBinder::bindAndValidateParameters(
                            constructor->getParameters(),
                            args,
                            "constructor for class '" + node->getClassName() + "'",
                            env,
                            node->getLocation()
                        );

                        // Execute constructor body
                        if (stmtEvaluator)
                        {
                            auto bodyPtr = constructor->getBody();
                            if (bodyPtr)
                            {
                                stmtEvaluator->evaluate(bodyPtr);
                            }
                        }
                    }
                    catch (...)
                    {
                        context->setCurrentInstance(prevInstance);
                        context->setInStaticMethod(wasInStaticMethod);
                        throw;
                    }
                    // Scope automatically exits via RAII
                }
                context->setCurrentInstance(prevInstance);
                context->setInStaticMethod(wasInStaticMethod);
            }
        }

        // Explicitly convert to Value type
        Value result = Value(instance);

        // Debug: Check what type we're actually returning
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(result))
        {
            // Good - we have an object
        }
        else if (std::holds_alternative<std::monostate>(result))
        {
            // Bad - we have void
            throw TypeException("Object creation returned void instead of instance");
        }
        else
        {
            throw TypeException("Object creation returned unexpected type");
        }

        return result;
    }

    std::shared_ptr<ObjectInstance> ObjectEvaluator::createInstance(
        const std::string& className,
        const std::vector<Value>& constructorArgs)
    {
        auto env = context->getEnvironment();
        return instanceManager->createInstance(className, constructorArgs, env);
    }

    std::shared_ptr<ObjectInstance> ObjectEvaluator::createInstanceWithTypeBindings(
        const std::string& className,
        const std::vector<Value>& constructorArgs,
        const std::unordered_map<std::string, std::string>& typeBindings)
    {
        auto env = context->getEnvironment();

        if (!env)
        {
            throw TypeException("Environment is null during object creation");
        }

        auto classDef = env->getClassRegistry()->findItem(className);
        if (!classDef)
        {
            throw UndefinedException("Class '" + className + "' is not defined");
        }

        // Create instance with generic type bindings
        auto instance = std::make_shared<ObjectInstance>(classDef, typeBindings);

        // Initialize fields with default values
        for (const auto& fieldPair : classDef->getInstanceFields())
        {
            auto field = fieldPair.second;
            instance->setField(field->getName(), field->getValue());
        }

        return instance;
    }

    Value ObjectEvaluator::evaluateMemberAccessNode(MemberAccessNode* node)
    {
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for member access");
        }

        // Evaluate the object expression
        Value objectValue = exprEvaluator->evaluate(node->getObject());

        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue))
        {
            auto instance = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
            return accessMember(instance, node->getMemberName());
        }
        else if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(objectValue))
        {
            auto array = std::get<std::shared_ptr<value::NativeArray>>(objectValue);
            if (node->getMemberName() == "length")
            {
                return static_cast<int>(array->size());
            }
            else
            {
                throw TypeException("Array does not have member '" + node->getMemberName() + "'");
            }
        }
        else
        {
            throw TypeException("Cannot access member '" + node->getMemberName() +
                "' on non-object value");
        }
    }

    Value ObjectEvaluator::accessMember(std::shared_ptr<ObjectInstance> object,
                                        const std::string& memberName)
    {
        if (!object)
        {
            throw TypeException("Cannot access member '" + memberName + "' on null object");
        }

        // VALIDATION: Prevent instance member access from static methods
        if (context->isInStaticMethodContext())
        {
            auto field = object->getField(memberName);
            if (field && !field->isStatic())
            {
                throw TypeException("Cannot access instance field '" + memberName +
                                    "' from static method context",
                                    SourceLocation()); // TODO: Pass proper location if available
            }
        }

        auto env = context->getEnvironment();
        return instanceManager->accessMember(object, memberName);
    }

    Value ObjectEvaluator::evaluateMemberAssignmentNode(MemberAssignmentNode* node)
    {
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for member assignment");
        }

        // Evaluate the object expression
        Value objectValue = exprEvaluator->evaluate(node->getObject());

        // Evaluate the new value
        Value newValue = exprEvaluator->evaluate(node->getValue());

        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue))
        {
            auto instance = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
            assignMember(instance, node->getMemberName(), newValue);
            return newValue;
        }
        else
        {
            throw TypeException("Cannot assign to member '" + node->getMemberName() +
                "' on non-object value");
        }
    }

    Value ObjectEvaluator::evaluateIndexAssignmentNode(IndexAssignmentNode* node)
    {
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for index assignment");
        }

        // Try to extract multi-dimensional assignment (e.g., arr[i][j][k] = value)
        auto multiDimResult = extractMultiDimensionalAssignment(node);
        if (multiDimResult.has_value())
        {
            auto [baseArray, indices] = multiDimResult.value();
            Value newValue = exprEvaluator->evaluate(node->getValue());
            return performDirectMultiDimensionalAssignment(baseArray, indices, newValue, node->getLocation());
        }

        // Check if this is a multi-dimensional assignment (e.g., arr2d[0][0] = value)
        // If node->getObject() is an IndexAccessNode, we might have a multi-dimensional assignment
        auto objectASTNode = node->getObject();

        // Try to detect if this is an IndexAccessNode
        if (auto indexAccessNode = dynamic_cast<ast::nodes::expressions::IndexAccessNode*>(objectASTNode))
        {
            // Get the base array (e.g., arr2d in arr2d[0][0] = value)
            Value baseArrayValue = exprEvaluator->evaluate(indexAccessNode->getCollection());

            if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(baseArrayValue))
            {
                auto baseArray = std::get<std::shared_ptr<value::FlatMultiArray>>(baseArrayValue);

                // Get the first dimension index (e.g., 0 in arr2d[0][0])
                Value firstIndexValue = exprEvaluator->evaluate(indexAccessNode->getIndex());

                // Get the second dimension index (e.g., 0 in arr2d[0][0])
                Value secondIndexValue = exprEvaluator->evaluate(node->getIndex());

                // Get the new value
                Value newValue = exprEvaluator->evaluate(node->getValue());

                if (std::holds_alternative<int>(firstIndexValue) && std::holds_alternative<int>(secondIndexValue))
                {
                    int firstIndex = std::get<int>(firstIndexValue);
                    int secondIndex = std::get<int>(secondIndexValue);

                    // Validate indices before conversion
                    if (firstIndex < 0)
                    {
                        throw TypeException(
                            "Multi-dimensional array first index " + std::to_string(firstIndex) + " is negative",
                            node->getLocation());
                    }
                    if (secondIndex < 0)
                    {
                        throw TypeException(
                            "Multi-dimensional array second index " + std::to_string(secondIndex) + " is negative",
                            node->getLocation());
                    }

                    std::vector<size_t> indices;
                    indices.push_back(static_cast<size_t>(firstIndex));
                    indices.push_back(static_cast<size_t>(secondIndex));

                    try
                    {
                        baseArray->set(indices, newValue);
                        return newValue;
                    }
                    catch (const std::out_of_range& e)
                    {
                        throw TypeException("Multi-dimensional array assignment failed: " + std::string(e.what()),
                                            node->getLocation());
                    }
                }
            }
            else if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(baseArrayValue))
            {
                auto baseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(baseArrayValue);

                // Get the first dimension index (e.g., 0 in arr2d[0][0])
                Value firstIndexValue = exprEvaluator->evaluate(indexAccessNode->getIndex());

                // Get the second dimension index (e.g., 0 in arr2d[0][0])
                Value secondIndexValue = exprEvaluator->evaluate(node->getIndex());

                // Get the new value
                Value newValue = exprEvaluator->evaluate(node->getValue());

                if (std::holds_alternative<int>(firstIndexValue) && std::holds_alternative<int>(secondIndexValue))
                {
                    int firstIndex = std::get<int>(firstIndexValue);
                    int secondIndex = std::get<int>(secondIndexValue);

                    // Validate indices before conversion
                    if (firstIndex < 0)
                    {
                        throw TypeException(
                            "Sparse multi-dimensional array first index " + std::to_string(firstIndex) + " is negative",
                            node->getLocation());
                    }
                    if (secondIndex < 0)
                    {
                        throw TypeException(
                            "Sparse multi-dimensional array second index " + std::to_string(secondIndex) +
                            " is negative", node->getLocation());
                    }

                    std::vector<size_t> indices;
                    indices.push_back(static_cast<size_t>(firstIndex));
                    indices.push_back(static_cast<size_t>(secondIndex));

                    try
                    {
                        baseArray->set(indices, newValue);
                        return newValue;
                    }
                    catch (const std::out_of_range& e)
                    {
                        throw TypeException(
                            "Sparse multi-dimensional array assignment failed: " + std::string(e.what()),
                            node->getLocation());
                    }
                }
            }
        }

        // Fall back to single-dimensional assignment
        // Evaluate the object expression (should be an array)
        Value objectValue = exprEvaluator->evaluate(node->getObject());

        // Evaluate the index expression
        Value indexValue = exprEvaluator->evaluate(node->getIndex());

        // Evaluate the new value
        Value newValue = exprEvaluator->evaluate(node->getValue());

        // Check for lambda-to-interface conversion for array assignments
        if (std::holds_alternative<std::shared_ptr<value::LambdaValue>>(newValue)) {
            // Try to determine the array element type to convert lambda accordingly
            if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(objectValue)) {
                auto nativeArray = std::get<std::shared_ptr<value::NativeArray>>(objectValue);

                // Check if the array has element type information
                if (nativeArray->getElementType() == ValueType::OBJECT && !nativeArray->getElementTypeName().empty()) {
                    // Try to convert lambda to the array's element interface type
                    if (stmtEvaluator) {
                        try {
                            newValue = stmtEvaluator->convertLambdaToInterface(newValue, nativeArray->getElementTypeName());
                        } catch (...) {
                            // If conversion fails, keep original lambda value
                        }
                    }
                }
            }
        }

        // Check if index is an integer
        if (!std::holds_alternative<int>(indexValue))
        {
            throw TypeException("Array index must be an integer", node->getLocation());
        }

        int index = std::get<int>(indexValue);

        // Check if object is a NativeArray
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(objectValue))
        {
            auto nativeArray = std::get<std::shared_ptr<value::NativeArray>>(objectValue);

            // Check bounds with descriptive error message
            if (index < 0)
            {
                throw TypeException(
                    "Array assignment index " + std::to_string(index) + " is negative (valid range: 0 to " +
                    std::to_string(nativeArray->size() - 1) + ")", node->getLocation());
            }
            if (static_cast<size_t>(index) >= nativeArray->size())
            {
                throw TypeException("Array assignment index " + std::to_string(index) + " exceeds array size of " +
                                    std::to_string(nativeArray->size()) + " elements (valid range: 0 to " +
                                    std::to_string(nativeArray->size() - 1) + ")", node->getLocation());
            }

            nativeArray->set(static_cast<size_t>(index), newValue);
            return newValue;
        }

        // Check if object is a FlatMultiArray (for multi-dimensional arrays)
        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(objectValue))
        {
            auto flatArray = std::get<std::shared_ptr<value::FlatMultiArray>>(objectValue);

            // Check bounds with descriptive error message
            if (index < 0)
            {
                throw TypeException(
                    "Multi-dimensional array assignment index " + std::to_string(index) +
                    " is negative (valid range: 0 to " +
                    std::to_string(flatArray->size() - 1) + ")", node->getLocation());
            }
            if (static_cast<size_t>(index) >= flatArray->size())
            {
                throw TypeException(
                    "Multi-dimensional array assignment index " + std::to_string(index) + " exceeds array size of " +
                    std::to_string(flatArray->size()) + " elements (valid range: 0 to " +
                    std::to_string(flatArray->size() - 1) + ")", node->getLocation());
            }

            // For 1D FlatMultiArray, set directly
            if (flatArray->getRank() == 1)
            {
                try
                {
                    flatArray->set(static_cast<size_t>(index), newValue);
                    return newValue;
                }
                catch (const std::out_of_range& e)
                {
                    throw TypeException("Array assignment failed: " + std::string(e.what()), node->getLocation());
                }
            }
            else
            {
                // For multi-dimensional arrays, this should be handled differently
                // The parser should create nested IndexAssignmentNodes, but if we get here,
                // it means we're assigning to a sub-array which isn't supported
                throw TypeException("Cannot assign array to index position", node->getLocation());
            }
        }

        // Check if object is a SparseMultiArray (for adaptive sparse arrays)
        if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(objectValue))
        {
            auto sparseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(objectValue);

            // Check bounds with descriptive error message
            if (index < 0)
            {
                throw TypeException(
                    "Sparse array assignment index " + std::to_string(index) + " is negative (valid range: 0 to " +
                    std::to_string(sparseArray->size() - 1) + ")", node->getLocation());
            }
            if (static_cast<size_t>(index) >= sparseArray->size())
            {
                throw TypeException(
                    "Sparse array assignment index " + std::to_string(index) + " exceeds array size of " +
                    std::to_string(sparseArray->size()) + " elements (valid range: 0 to " +
                    std::to_string(sparseArray->size() - 1) + ")", node->getLocation());
            }

            // For single dimension sparse array
            if (sparseArray->getRank() == 1)
            {
                try
                {
                    std::vector<size_t> indices = {static_cast<size_t>(index)};
                    sparseArray->set(indices, newValue);
                    return newValue;
                }
                catch (const std::out_of_range& e)
                {
                    throw TypeException("Sparse array assignment failed: " + std::string(e.what()),
                                        node->getLocation());
                }
            }
            else
            {
                // For multi-dimensional sparse arrays, this should be handled differently
                // The parser should create nested IndexAssignmentNodes
                throw TypeException("Cannot assign array to index position in sparse array", node->getLocation());
            }
        }

        throw TypeException("Cannot assign to index of non-array value", node->getLocation());
        /* if (std::holds_alternative<std::shared_ptr<Array>>(objectValue))
        {
            auto array = std::get<std::shared_ptr<Array>>(objectValue);

            if (std::holds_alternative<int>(indexValue))
            {
                int index = std::get<int>(indexValue);
                array->set(index, newValue);

                return newValue;
            }
            else
            {
                throw TypeException("Array index must be an integer");
            }
        }
        else
        {
            throw TypeException("Cannot assign to index on non-array value");
        } */
    }

    void ObjectEvaluator::assignMember(std::shared_ptr<ObjectInstance> object,
                                       const std::string& memberName,
                                       const Value& value)
    {
        if (!object)
        {
            throw TypeException("Cannot assign to member '" + memberName + "' on null object");
        }

        // VALIDATION: Prevent instance member assignment from static methods
        if (context->isInStaticMethodContext())
        {
            auto field = object->getField(memberName);
            if (field && !field->isStatic())
            {
                throw TypeException("Cannot assign to instance field '" + memberName +
                                    "' from static method context",
                                    SourceLocation()); // TODO: Pass proper location if available
            }
        }

        instanceManager->assignMember(object, memberName, value);
    }

    Value ObjectEvaluator::evaluateMethodCallNode(MethodCallNode* node)
    {
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for method call");
        }

        // Evaluate arguments first (needed for both static and instance calls)
        std::vector<Value> args = evaluateArgumentList(node->getArguments());

        // Handle static method calls
        if (node->getIsStaticCall())
        {
            // For static calls, the object should be a VariableNode containing the class name
            if (auto varNode = dynamic_cast<nodes::expressions::VariableNode*>(node->getObject()))
            {
                std::string className = varNode->getName();

                // Call static method with or without generic type arguments
                if (node->hasGenericTypeArguments())
                {
                    return callStaticMethod(className, node->getMethodName(), args,
                                            node->getGenericTypeArguments(), node->getLocation());
                }
                else
                {
                    return callStaticMethod(className, node->getMethodName(), args, node->getLocation());
                }
            }
            else
            {
                throw TypeException("Invalid static method call - expected class name");
            }
        }

        // Handle instance method calls
        Value objectValue;
        if (dynamic_cast<NewNode*>(node->getObject()))
        {
            // Handle NewNode directly since ExpressionEvaluator would delegate back to us anyway
            objectValue = evaluate(node->getObject());
        }
        else
        {
            // Delegate to expression evaluator for other nodes (like VariableNode)
            objectValue = exprEvaluator->evaluate(node->getObject());
        }

        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue))
        {
            auto instance = std::get<std::shared_ptr<ObjectInstance>>(objectValue);

            // Check if this is a lambda-backed interface (has lambda field)
            auto lambdaField = instance->getField(constants::lambda::LAMBDA_FIELD_NAME);
            if (lambdaField)
            {
                Value lambdaValue = lambdaField->getValue();
                if (std::holds_alternative<std::shared_ptr<value::LambdaValue>>(lambdaValue))
                {
                    auto lambda = std::get<std::shared_ptr<value::LambdaValue>>(lambdaValue);
                    if (!lambda) {
                        throw TypeException("Lambda value is null - cannot invoke method '" +
                                          node->getMethodName() + "'");
                    }
                    // Invoke the lambda with the method arguments
                    Value result = lambda->invoke(args, context);
                    return result;
                }
            }

            return callMethod(instance, node->getMethodName(), args, node->getLocation());
        }
        // Collection method dispatch removed - collections now implemented in mType language
        // Collections will be handled as regular object method calls
        else
        {
            throw TypeException("Cannot call method '" + node->getMethodName() +
                "' on non-object value");
        }
    }

    Value ObjectEvaluator::callMethod(std::shared_ptr<ObjectInstance> object,
                                      const std::string& methodName,
                                      const std::vector<Value>& args,
                                      const errors::SourceLocation& location)
    {
        // Check if this is a lambda-backed interface (has lambda field)
        Value lambdaValue = object->getFieldValue(constants::lambda::LAMBDA_FIELD_NAME);
        if (!std::holds_alternative<std::monostate>(lambdaValue))
        {
            if (std::holds_alternative<std::shared_ptr<value::LambdaValue>>(lambdaValue))
            {
                auto lambda = std::get<std::shared_ptr<value::LambdaValue>>(lambdaValue);
                if (!lambda) {
                    throw TypeException("Lambda value is null - cannot invoke interface method '" +
                                      methodName + "'");
                }
                // Invoke the lambda with the method arguments
                Value result = lambda->invoke(args, context);
                return result;
            }
        }

        auto env = context->getEnvironment();

        // Get the class definition from the object
        auto classDef = object->getClassDefinition();
        if (!classDef)
        {
            throw UndefinedException("Object has no class definition");
        }

        // Find the method
        auto method = classDef->findMethod(methodName, args.size());
        if (!method)
        {
            throw UndefinedException("Method '" + methodName + "' not found in class '" +
                classDef->getName() + "'");
        }

        // Convert lambda arguments to interface implementations if needed
        std::vector<Value> convertedArgs = args;
        if (stmtEvaluator) {
            const auto& parameters = method->getParameters();
            for (size_t i = 0; i < convertedArgs.size() && i < parameters.size(); i++) {
                // Check if argument is a lambda and parameter expects an interface
                if (std::holds_alternative<std::shared_ptr<value::LambdaValue>>(convertedArgs[i])) {
                    const auto& param = parameters[i];
                    const auto& paramType = param.second; // ParameterType
                    if (paramType.basicType == ValueType::OBJECT) {
                        // Try to convert lambda to interface implementation
                        std::string targetName;
                        if (paramType.interfaceName.has_value()) {
                            targetName = paramType.interfaceName.value();
                        } else if (paramType.className.has_value()) {
                            targetName = paramType.className.value();
                        }

                        if (!targetName.empty()) {
                            try {
                                convertedArgs[i] = stmtEvaluator->convertLambdaToInterface(
                                    convertedArgs[i], targetName);
                            } catch (...) {
                                // If conversion fails, keep original lambda value
                                // The error will be caught during parameter validation
                            }
                        }
                    }
                }
            }
        }

        // Check if this method has a lambda implementation (lambda-to-interface scenario)
        if (method->hasLambdaNode())
        {
            auto lambdaNode = method->getLambdaNode();
            if (lambdaNode && method->isLambdaNodeValid())
            {
                // Create LambdaValue with current evaluation context
                // Convert shared_ptr to raw pointer for LambdaValue constructor
                auto rawLambdaNode = lambdaNode.get();
                if (!rawLambdaNode) {
                    throw TypeException("Lambda node is null - cannot create lambda for method '" + methodName + "'");
                }
                auto lambdaValue = std::make_shared<value::LambdaValue>(rawLambdaNode, context);

                // Set interface implementation info
                lambdaValue->setInterfaceImplementation(classDef->getName(), methodName);

                // Invoke the lambda with proper parameter mapping
                try {
                    Value result = lambdaValue->invoke(convertedArgs, context);
                    return result;
                } catch (const std::exception& e) {
                    throw TypeException("Lambda invocation failed: " + std::string(e.what()));
                }
            }
        }

        // Check if trying to call static method on instance
        if (method->isStatic())
        {
            throw TypeException("Cannot call static method '" + methodName +
                "' on instance of class '" + classDef->getName() +
                "'. Use class name instead.");
        }

        // VALIDATION: Prevent instance method calls from static methods only when calling on 'this'
        // Allow instance method calls on local objects and parameters within static methods
        if (context->isInStaticMethodContext() && !method->isStatic())
        {
            // Check if we're trying to call an instance method on the current instance ('this')
            // In static methods, currentInstance should be null, so any valid object calls are allowed
            auto currentInstance = context->getCurrentInstance();
            if (currentInstance && currentInstance == object)
            {
                throw TypeException("Cannot call instance method '" + methodName +
                                    "' on 'this' from static method context",
                                    SourceLocation()); // TODO: Pass proper location if available
            }
            // Allow calls on other objects (local variables, parameters, newly created objects)
        }


        // Set current instance context
        auto prevInstance = context->getCurrentInstance();
        context->setCurrentInstance(object);

        // Set generic type bindings from the object instance for method execution
        auto prevGenericBindings = context->getGenericTypeBindings();
        if (object && !object->getGenericTypeBindings().empty())
        {
            context->setGenericTypeBindings(object->getGenericTypeBindings());
        }

        // Temporarily clear static method context for instance method execution
        // Instance methods should run in instance context regardless of where they're called from
        bool wasInStaticMethod = context->isInStaticMethodContext();
        context->setInStaticMethod(false);

        // Use ScopeGuard and ParameterBinder utilities
        {
            utils::ScopeGuard scope(env, methodName, environment::manager::ScopeType::FUNCTION);

            try
            {
                // Use enhanced ParameterBinder for generic-aware parameter binding
                if (method->hasGenericInformation())
                {
                    // Use the new generic-aware parameter binding
                    utils::ParameterBinder::bindAndValidateParameters(
                        method,
                        convertedArgs,
                        "method '" + methodName + "'",
                        env,
                        location
                    );
                }
                else
                {
                    // Use legacy parameter binding for non-generic methods
                    utils::ParameterBinder::bindAndValidateParameters(
                        method->getParameters(),
                        convertedArgs,
                        "method '" + methodName + "'",
                        env,
                        location
                    );
                }

                // Store current class name for static field access from instance methods
                auto classNameVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    "__current_class_name__", ValueType::STRING, classDef->getName(), false
                );
                env->declareVariable("__current_class_name__", classNameVar);


                // Execute method body
                Value result = std::monostate{}; // void default
                if (method->getBody() && stmtEvaluator)
                {
                    stmtEvaluator->evaluate(method->getBody());
                }

                // Restore static method context
                context->setInStaticMethod(wasInStaticMethod);

                // Get return value if method returned
                if (context->shouldReturn())
                {
                    result = context->getReturnValue();
                    context->setReturned(false);
                }


                context->setCurrentInstance(prevInstance);

                // Restore previous generic type bindings
                context->setGenericTypeBindings(prevGenericBindings);

                return result;
            }
            catch (const exception::ReturnException& e)
            {
                // Handle return exception - extract return value
                context->setInStaticMethod(wasInStaticMethod); // Restore static method context
                context->setCurrentInstance(prevInstance);
                context->setGenericTypeBindings(prevGenericBindings); // Restore generic bindings
                context->setReturned(false); // Reset return state after handling exception
                return e.returnValue;
            }
            catch (...)
            {
                context->setInStaticMethod(wasInStaticMethod); // Restore static method context
                context->setCurrentInstance(prevInstance);
                context->setGenericTypeBindings(prevGenericBindings); // Restore generic bindings
                throw;
            }
            // Scope automatically exits via RAII
        }
    }

    std::vector<Value> ObjectEvaluator::evaluateArgumentList(
        const std::vector<std::unique_ptr<ASTNode>>& args)
    {
        std::vector<Value> values;
        values.reserve(args.size());

        if (exprEvaluator)
        {
            for (const auto& arg : args)
            {
                values.push_back(exprEvaluator->evaluate(arg.get()));
            }
        }

        return values;
    }

    // Static member operations (delegating to instance manager)
    Value ObjectEvaluator::accessStaticMember(const std::string& className,
                                              const std::string& memberName)
    {
        auto env = context->getEnvironment();
        return instanceManager->accessStaticMember(className, memberName, env);
    }

    void ObjectEvaluator::assignStaticMember(const std::string& className,
                                             const std::string& memberName,
                                             const Value& value)
    {
        auto env = context->getEnvironment();
        instanceManager->assignStaticMember(className, memberName, value, env);
    }

    Value ObjectEvaluator::callStaticMethod(const std::string& className,
                                            const std::string& methodName,
                                            const std::vector<Value>& args,
                                            const errors::SourceLocation& location)
    {
        auto env = context->getEnvironment();

        // Declare previousMethod for restoration in exception handlers
        auto previousMethod = context->getCurrentMethod();

        // Try to resolve type parameters from current object context first
        std::string resolvedClassName = className;
        auto currentInstance = context->getCurrentInstance();
        if (currentInstance)
        {
            auto instanceClassDef = currentInstance->getClassDefinition();
            if (instanceClassDef)
            {
                std::string instanceClassName = instanceClassDef->getName(); // e.g., "Set<int>"

                // Check if the target className contains type parameters that need resolution
                if (className.find('<') != std::string::npos && className.find('T') != std::string::npos)
                {
                    if (utils::GenericTypeManager::isGenericInstantiation(instanceClassName))
                    {
                        auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(
                            instanceClassName);

                        // Replace type parameters in className
                        resolvedClassName = className;
                        if (className.find("T") != std::string::npos && !typeArguments.empty())
                        {
                            // Simple T replacement - can be extended for multiple type parameters
                            size_t pos = resolvedClassName.find("T");
                            while (pos != std::string::npos)
                            {
                                resolvedClassName.replace(pos, 1, typeArguments[0]);
                                pos = resolvedClassName.find("T", pos + typeArguments[0].length());
                            }
                        }
                    }
                }
            }
        }

        // Find the class and method
        auto classDef = env->findClass(resolvedClassName);
        if (!classDef)
        {
            throw UndefinedException("Class '" + resolvedClassName + "' not found");
        }

        // Try to find method with exact argument count first
        auto method = classDef->findMethod(methodName, args.size());

        // If not found, try without argument count matching as a fallback
        if (!method)
        {
            method = classDef->getMethod(methodName);
        }

        // Check if method exists and is static
        if (!method)
        {
            throw UndefinedException("Method '" + methodName + "' not found in class '" + className + "'");
        }

        if (!method->isStatic())
        {
            throw UndefinedException("Method '" + methodName + "' in class '" + className + "' is not static");
        }

        // Use ScopeGuard and ParameterBinder utilities
        {
            utils::ScopeGuard scope(env, methodName, environment::manager::ScopeType::FUNCTION);

            try
            {
                // Use ParameterBinder utility for consistent parameter validation and binding
                utils::ParameterBinder::bindAndValidateParameters(
                    method->getParameters(),
                    args,
                    "static method '" + className + "::" + methodName + "'",
                    env,
                    location
                );

                // Store current class name for static field access
                auto classNameVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    "__current_class_name__", ValueType::STRING, className, false
                );
                env->declareVariable("__current_class_name__", classNameVar);

                // Set static method context to prevent instance member access
                bool previousStaticState = context->isInStaticMethodContext();
                context->setInStaticMethod(true);

                try
                {
                    // Execute method body (no instance context for static methods)
                    Value result = std::monostate{}; // void default
                    if (method->getBody() && stmtEvaluator)
                    {
                        stmtEvaluator->evaluate(method->getBody());
                    }

                    // Get return value if method returned
                    if (context->shouldReturn())
                    {
                        result = context->getReturnValue();
                        context->setReturned(false);
                    }

                    // Restore previous static method state
                    context->setInStaticMethod(previousStaticState);
                    context->setCurrentMethod(previousMethod);
                    return result;
                }
                catch (const exception::ReturnException& e)
                {
                    // Handle return exception - extract return value
                    context->setInStaticMethod(previousStaticState);
                    context->setCurrentMethod(previousMethod);
                    context->setReturned(false);
                    return e.returnValue;
                }
                catch (...)
                {
                    // Ensure we restore state even if exception occurs
                    context->setInStaticMethod(previousStaticState);
                    context->setCurrentMethod(previousMethod);
                    throw;
                }
            }
            catch (...)
            {
                throw; // Re-throw any exceptions that weren't handled by inner try-catch
            }
            // Scope automatically exits via RAII
        }
    }

    Value ObjectEvaluator::callStaticMethod(const std::string& className,
                                            const std::string& methodName,
                                            const std::vector<Value>& args,
                                            const std::vector<std::string>& genericTypeArguments,
                                            const errors::SourceLocation& location)
    {
        auto env = context->getEnvironment();

        // Get the class definition
        auto classDef = env->getClassRegistry()->findItem(className);
        if (!classDef)
        {
            throw UndefinedException("Class '" + className + "' not found for static method call");
        }

        // Find the static method
        auto method = classDef->getMethod(methodName);
        if (!method)
        {
            throw UndefinedException("Static method '" + methodName + "' not found in class '" + className + "'");
        }

        if (!method->isStatic())
        {
            throw UndefinedException("Method '" + methodName + "' in class '" + className + "' is not static");
        }

        // Handle generic method instantiation
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> methodToCall = method;

        if (!genericTypeArguments.empty())
        {
            // Validate that the method is actually generic
            if (!method->hasGenericInformation())
            {
                throw TypeException(
                    "Method '" + methodName + "' is not generic but generic type arguments were provided");
            }

            // Validate type arguments
            if (!utils::GenericTypeManager::validateStaticMethodTypeArguments(method, genericTypeArguments))
            {
                throw TypeException("Invalid type arguments for static generic method '" +
                    className + "::" + methodName + "'");
            }

            // Create a cache key for the instantiated method
            std::string signatureKey = utils::GenericTypeManager::createStaticMethodSignatureKey(
                className, methodName, genericTypeArguments);

            // Check if we already have this instantiation cached
            static std::mutex staticGenericMethodCacheMutex;
            static std::unordered_map<std::string, std::shared_ptr<runtimeTypes::klass::MethodDefinition>>
                staticGenericMethodCache;

            {
                std::lock_guard<std::mutex> lock(staticGenericMethodCacheMutex);

                auto cacheIt = staticGenericMethodCache.find(signatureKey);
                if (cacheIt != staticGenericMethodCache.end())
                {
                    methodToCall = cacheIt->second;
                }
                else
                {
                    // Instantiate the generic method
                    methodToCall = utils::GenericTypeManager::instantiateStaticGenericMethod(
                        method, genericTypeArguments);

                    // Estimate memory usage for the instantiated method
                    size_t estimatedMemory = sizeof(runtimeTypes::klass::MethodDefinition) +
                        signatureKey.size() +
                        (methodToCall->getParameters().size() * sizeof(std::pair<std::string, value::ValueType>));

                    // Cache the instantiated method
                    staticGenericMethodCache[signatureKey] = methodToCall;
                }
            }
        }

        // Use ScopeGuard and ParameterBinder utilities
        {
            utils::ScopeGuard scope(env, methodName, environment::manager::ScopeType::FUNCTION);

            try
            {
                // Use ParameterBinder utility for consistent parameter validation and binding
                if (methodToCall->hasGenericInformation())
                {
                    // Use generic-aware parameter binding for instantiated generic methods
                    utils::ParameterBinder::bindAndValidateParameters(
                        methodToCall,
                        args,
                        "static method '" + className + "::" + methodName + "'",
                        env,
                        location
                    );
                }
                else
                {
                    // Use legacy parameter binding for non-generic methods
                    utils::ParameterBinder::bindAndValidateParameters(
                        methodToCall->getParameters(),
                        args,
                        "static method '" + className + "::" + methodName + "'",
                        env,
                        location
                    );
                }

                // Store current class name for static field access
                auto classNameVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    "__current_class_name__", ValueType::STRING, className, false
                );
                env->declareVariable("__current_class_name__", classNameVar);

                // Set static method context to prevent instance member access
                bool previousStaticState = context->isInStaticMethodContext();
                context->setInStaticMethod(true);

                // Set current method context for generic type resolution
                auto previousMethod = context->getCurrentMethod();
                context->setCurrentMethod(methodToCall);

                try
                {
                    // Execute method body (no instance context for static methods)
                    Value result = std::monostate{}; // void default
                    if (methodToCall->getBody() && stmtEvaluator)
                    {
                        stmtEvaluator->evaluate(methodToCall->getBody());
                    }

                    // Get return value if method returned
                    if (context->shouldReturn())
                    {
                        result = context->getReturnValue();
                        context->setReturned(false);
                    }

                    // Restore previous static method state
                    context->setInStaticMethod(previousStaticState);
                    context->setCurrentMethod(previousMethod);
                    return result;
                }
                catch (const exception::ReturnException& e)
                {
                    // Handle return exception - extract return value
                    context->setInStaticMethod(previousStaticState);
                    context->setCurrentMethod(previousMethod);
                    context->setReturned(false);
                    return e.returnValue;
                }
                catch (...)
                {
                    // Ensure we restore state even if exception occurs
                    context->setInStaticMethod(previousStaticState);
                    context->setCurrentMethod(previousMethod);
                    throw;
                }
            }
            catch (...)
            {
                throw; // Re-throw any exceptions that weren't handled by inner try-catch
            }
            // Scope automatically exits via RAII
        }
    }

    // Placeholder implementations for complex methods
    Value ObjectEvaluator::evaluateMethodNode(MethodNode* node)
    {
        throw TypeException("Method definition evaluation not implemented in refactored version");
    }

    Value ObjectEvaluator::evaluateFieldNode(FieldNode* node)
    {
        throw TypeException("Field definition evaluation not implemented in refactored version");
    }

    Value ObjectEvaluator::evaluateConstructorNode(ConstructorNode* node)
    {
        throw TypeException("Constructor definition evaluation not implemented in refactored version");
    }

    std::string ObjectEvaluator::resolveTypeParameterFromContext(const std::string& typeParam)
    {
        // First try to resolve from current method context (for static generic methods)
        auto currentMethod = context->getCurrentMethod();
        if (currentMethod && currentMethod->hasGenericInformation())
        {
            const auto& substitutionMap = currentMethod->getTypeSubstitutionMap();
            auto it = substitutionMap.find(typeParam);
            if (it != substitutionMap.end())
            {
                return it->second;
            }
        }

        // Try to resolve type parameters from the current object instance context
        auto currentInstance = context->getCurrentInstance();
        if (!currentInstance)
        {
            return typeParam;
        }

        auto classDef = currentInstance->getClassDefinition();
        if (!classDef)
        {
            return typeParam;
        }

        // Get the class name which should contain the instantiated type
        // e.g., "Set<int>" for a Set<int> instance
        std::string className = classDef->getName();

        // Check if the class name itself is a generic instantiation (e.g., "Set<int>")
        // This handles the case where we have an instantiated generic class
        if (!classDef->isGeneric() && !utils::GenericTypeManager::isGenericInstantiation(className))
        {
            return typeParam;
        }

        // Parse the generic instantiation to extract type parameters
        if (utils::GenericTypeManager::isGenericInstantiation(className))
        {
            auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(className);

            // Get the generic class definition to find its type parameter names
            auto env = context->getEnvironment();
            auto genericClass = env->findClass(baseName);
            if (genericClass && genericClass->isGeneric())
            {
                auto genericParams = genericClass->getGenericParameters();

                // Handle array types (T[], T[][], Element[], etc.)
                if (typeParam.find("[]") != std::string::npos)
                {
                    std::string baseType = typeParam.substr(0, typeParam.find("[]"));
                    std::string arraySuffix = typeParam.substr(typeParam.find("[]"));

                    // Find the position of baseType in the generic parameters
                    for (size_t i = 0; i < genericParams.size() && i < typeArguments.size(); ++i)
                    {
                        if (genericParams[i].name == baseType)
                        {
                            return typeArguments[i] + arraySuffix;
                        }
                    }
                }
                // For simple cases like Set<T>, map any type parameter to its corresponding argument
                else
                {
                    // Find the position of typeParam in the generic parameters
                    for (size_t i = 0; i < genericParams.size() && i < typeArguments.size(); ++i)
                    {
                        if (genericParams[i].name == typeParam)
                        {
                            return typeArguments[i];
                        }
                    }
                }
            }
        }

        // If we can't resolve, return the original parameter
        return typeParam;
    }

    std::optional<std::pair<Value, std::vector<size_t>>> ObjectEvaluator::extractMultiDimensionalAssignment(
        IndexAssignmentNode* node)
    {
        // Recursively extract all indices from nested IndexAccessNode chain
        std::vector<ASTNode*> indexNodes;
        ASTNode* currentNode = node->getObject();

        // Traverse the chain of IndexAccessNode to collect all index expressions
        // For arr[i][j][k] = value, we traverse from outer to inner, inserting at beginning for correct order
        while (auto indexAccess = dynamic_cast<ast::nodes::expressions::IndexAccessNode*>(currentNode))
        {
            indexNodes.insert(indexNodes.begin(), indexAccess->getIndex());
            currentNode = indexAccess->getCollection();
        }

        // Add the final index from the IndexAssignmentNode itself
        indexNodes.push_back(node->getIndex());

        // We need at least 3 indices for enhanced multi-dimensional assignment
        // 2D assignments should continue using the existing logic
        if (indexNodes.size() < 3)
        {
            return std::nullopt;
        }

        // Evaluate the base array (the deepest collection)
        Value baseArray = exprEvaluator->evaluate(currentNode);

        // Determine the expected dimensions based on array type
        size_t expectedRank = 0;
        bool isMultiDimensional = false;

        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(baseArray))
        {
            auto flatArray = std::get<std::shared_ptr<value::FlatMultiArray>>(baseArray);
            expectedRank = flatArray->getRank();
            isMultiDimensional = true;
        }
        else if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(baseArray))
        {
            auto sparseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(baseArray);
            expectedRank = sparseArray->getRank();
            isMultiDimensional = true;
        }
        else if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(baseArray))
        {
            // For jagged arrays, we support arbitrary nesting depth
            // The number of indices should match the nesting depth
            expectedRank = indexNodes.size();
            isMultiDimensional = true;
        }

        if (!isMultiDimensional)
        {
            return std::nullopt;
        }

        // For fixed-dimension arrays, verify index count matches expected rank
        if ((std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(baseArray) ||
             std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(baseArray)) &&
            indexNodes.size() != expectedRank)
        {
            return std::nullopt;
        }

        // Extract and validate all indices
        std::vector<size_t> indices;
        indices.reserve(indexNodes.size());

        // Process indices in forward order (they are now in correct order due to insert-at-beginning)
        for (auto it = indexNodes.begin(); it != indexNodes.end(); ++it)
        {
            Value indexValue = exprEvaluator->evaluate(*it);

            if (!std::holds_alternative<int>(indexValue))
            {
                return std::nullopt;
            }

            int index = std::get<int>(indexValue);
            if (index < 0)
            {
                return std::nullopt;
            }

            indices.push_back(static_cast<size_t>(index));
        }

        return std::make_pair(baseArray, indices);
    }

    Value ObjectEvaluator::performDirectMultiDimensionalAssignment(const Value& baseArray,
                                                                   const std::vector<size_t>& indices,
                                                                   const Value& newValue,
                                                                   const SourceLocation& location)
    {
        // Handle FlatMultiArray direct assignment
        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(baseArray))
        {
            auto flatArray = std::get<std::shared_ptr<value::FlatMultiArray>>(baseArray);

            try
            {
                flatArray->set(indices, newValue);
                return newValue;
            }
            catch (const std::out_of_range& e)
            {
                throw TypeException("Multi-dimensional array assignment failed: " + std::string(e.what()), location);
            }
        }

        // Handle SparseMultiArray direct assignment
        if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(baseArray))
        {
            auto sparseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(baseArray);

            try
            {
                sparseArray->set(indices, newValue);
                return newValue;
            }
            catch (const std::out_of_range& e)
            {
                throw TypeException("Sparse multi-dimensional array assignment failed: " + std::string(e.what()),
                                    location);
            }
        }

        // Handle NativeArray (jagged arrays) multi-dimensional assignment
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(baseArray))
        {
            if (indices.empty())
            {
                throw TypeException("No indices provided for jagged array assignment", location);
            }

            try
            {
                // Navigate through nested NativeArrays for all but the last index
                Value currentArray = baseArray;
                for (size_t i = 0; i < indices.size() - 1; ++i)
                {
                    if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(currentArray))
                    {
                        throw TypeException("Expected nested array at dimension " + std::to_string(i) +
                                          " but found different type", location);
                    }

                    auto nativeArray = std::get<std::shared_ptr<value::NativeArray>>(currentArray);
                    size_t index = indices[i];

                    if (index >= nativeArray->size())
                    {
                        throw TypeException("Index " + std::to_string(index) + " out of bounds for dimension " +
                                          std::to_string(i) + " (size: " + std::to_string(nativeArray->size()) + ")",
                                          location);
                    }

                    currentArray = nativeArray->get(index);

                    // Check if we got null at an intermediate level
                    if (std::holds_alternative<std::monostate>(currentArray))
                    {
                        throw TypeException("Null array encountered at dimension " + std::to_string(i) +
                                          ". Initialize sub-array before assignment.", location);
                    }
                }

                // Handle the final assignment
                if (!std::holds_alternative<std::shared_ptr<value::NativeArray>>(currentArray))
                {
                    throw TypeException("Expected array for final assignment but found different type", location);
                }

                auto finalArray = std::get<std::shared_ptr<value::NativeArray>>(currentArray);
                size_t finalIndex = indices.back();

                if (finalIndex >= finalArray->size())
                {
                    throw TypeException("Final index " + std::to_string(finalIndex) + " out of bounds " +
                                      "(size: " + std::to_string(finalArray->size()) + ")", location);
                }

                finalArray->set(finalIndex, newValue);
                return newValue;
            }
            catch (const TypeException&)
            {
                throw; // Re-throw TypeExceptions as-is
            }
            catch (const std::exception& e)
            {
                throw TypeException("Jagged array assignment failed: " + std::string(e.what()), location);
            }
        }

        throw TypeException("Unsupported array type for direct multi-dimensional assignment", location);
    }

    void ObjectEvaluator::validateInterfaceImplementations(std::shared_ptr<ClassDefinition> classDef, ClassNode* node)
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

            // Create type substitution map for generic resolution
            std::unordered_map<std::string, std::string> typeSubstitutions;
            const auto& interfaceGenericParams = interfaceDef->getGenericParameters();

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
                std::string resolvedReturnType = resolveGenericType(signature.returnType->getBaseTypeName(), typeSubstitutions);
                std::string methodReturnType;

                // Use generic return type if available, otherwise fall back to basic ValueType
                if (method->getGenericReturnType())
                {
                    methodReturnType = method->getGenericReturnType()->getBaseTypeName();
                }
                else
                {
                    methodReturnType = valueTypeToString(method->getReturnType());
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
                        methodParamType = valueTypeToString(methodParams[i].second);
                    }

                    std::string resolvedParamType = resolveGenericType(signature.parameters[i].second->getBaseTypeName(), typeSubstitutions);

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

    std::pair<std::string, std::vector<std::string>> ObjectEvaluator::parseGenericInterfaceName(const std::string& interfaceName)
    {
        // Check if this is a generic interface (contains '<' and '>')
        size_t openBracket = interfaceName.find('<');
        if (openBracket == std::string::npos) {
            // Non-generic interface
            return {interfaceName, {}};
        }

        size_t closeBracket = interfaceName.find('>', openBracket);
        if (closeBracket == std::string::npos) {
            throw TypeException("Malformed generic interface name: " + interfaceName);
        }

        std::string baseInterfaceName = interfaceName.substr(0, openBracket);
        std::string typeArgsStr = interfaceName.substr(openBracket + 1, closeBracket - openBracket - 1);

        std::vector<std::string> typeArguments;

        // Parse type arguments (simple comma-separated parsing)
        if (!typeArgsStr.empty()) {
            std::stringstream ss(typeArgsStr);
            std::string typeArg;
            while (std::getline(ss, typeArg, ',')) {
                // Trim whitespace
                typeArg.erase(0, typeArg.find_first_not_of(" \t"));
                typeArg.erase(typeArg.find_last_not_of(" \t") + 1);
                if (!typeArg.empty()) {
                    typeArguments.push_back(typeArg);
                }
            }
        }

        return {baseInterfaceName, typeArguments};
    }

    std::string ObjectEvaluator::resolveGenericType(const std::string& typeName, const std::unordered_map<std::string, std::string>& typeSubstitutions)
    {
        // Check if this type needs to be substituted
        auto it = typeSubstitutions.find(typeName);
        if (it != typeSubstitutions.end()) {
            return it->second;
        }

        // If no substitution found, return the original type name
        return typeName;
    }

    std::string ObjectEvaluator::valueTypeToString(const value::ValueType& type)
    {
        switch (type) {
            case value::ValueType::INT: return "int";
            case value::ValueType::FLOAT: return "float";
            case value::ValueType::BOOL: return "bool";
            case value::ValueType::STRING: return "string";
            case value::ValueType::VOID: return "void";
            case value::ValueType::OBJECT: return "object";
            case value::ValueType::NULL_TYPE: return "null";
            default: return "unknown";
        }
    }
}
