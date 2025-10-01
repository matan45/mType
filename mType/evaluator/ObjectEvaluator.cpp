#include "ObjectEvaluator.hpp"
#include "utils/NodeTypeRegistry.hpp"
#include "objects/ArrayAssignmentHandler.hpp"
#include "objects/ClassRegistrationHandler.hpp"
#include "objects/StaticMemberHandler.hpp"
#include "objects/InstanceOperationHandler.hpp"
#include "objects/GenericInstantiationHandler.hpp"
#include "ExpressionEvaluator.hpp"
#include "StatementEvaluator.hpp"
#include "../constants/LambdaConstants.hpp"
#include "../value/LambdaValue.hpp"
#include "utils/ParameterBinder.hpp"
#include "utils/GenericTypeManager.hpp"
#include "utils/ArgumentEvaluator.hpp"
#include "../value/FlatMultiArray.hpp"
#include "../value/SparseMultiArray.hpp"
#include "../errors/TypeException.hpp"
#include "../ast/nodes/classes/FieldNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../ast/nodes/classes/ConstructorNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/classes/InterfaceNode.hpp"
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

namespace evaluator
{
    using namespace errors;
    using namespace runtimeTypes::klass;
    using namespace parser;

    ObjectEvaluator::ObjectEvaluator(std::shared_ptr<EvaluationContext> ctx)
        : context(ctx)
        , instanceManager(std::make_unique<InstanceManager>())
        , arrayAssignmentHandler(std::make_unique<objects::ArrayAssignmentHandler>(ctx))
        , classRegistrationHandler(std::make_unique<objects::ClassRegistrationHandler>(ctx))
        , staticMemberHandler(std::make_unique<objects::StaticMemberHandler>(ctx, instanceManager.get()))
        , instanceOperationHandler(std::make_unique<objects::InstanceOperationHandler>(ctx, instanceManager.get()))
        , genericInstantiationHandler(std::make_unique<objects::GenericInstantiationHandler>(ctx))
        , exprEvaluator(nullptr)
        , stmtEvaluator(nullptr)
    {
        // Set back-reference so handler can call back to ObjectEvaluator
        genericInstantiationHandler->setObjectEvaluator(this);

        // Initialize dispatcher with all node type handlers
        initializeDispatcher();
    }

    ObjectEvaluator::~ObjectEvaluator() = default;

    Value ObjectEvaluator::evaluate(ASTNode* node)
    {
        if (!node || !canHandle(node))
        {
            return std::monostate{};
        }

        // Use dispatcher for O(1) lookup instead of O(n) dynamic_cast chain
        return dispatcher.dispatch(this, node);
    }

    void ObjectEvaluator::initializeDispatcher()
    {
        // Register all object node handlers with the dispatcher
        dispatcher.registerMethod<ClassNode>(&ObjectEvaluator::evaluateClassNode);
        dispatcher.registerMethod<InterfaceNode>(&ObjectEvaluator::evaluateInterfaceNode);
        dispatcher.registerMethod<MethodNode>(&ObjectEvaluator::evaluateMethodNode);
        dispatcher.registerMethod<FieldNode>(&ObjectEvaluator::evaluateFieldNode);
        dispatcher.registerMethod<ConstructorNode>(&ObjectEvaluator::evaluateConstructorNode);
        dispatcher.registerMethod<NewNode>(&ObjectEvaluator::evaluateNewNode);
        dispatcher.registerMethod<MemberAccessNode>(&ObjectEvaluator::evaluateMemberAccessNode);
        dispatcher.registerMethod<MethodCallNode>(&ObjectEvaluator::evaluateMethodCallNode);
        dispatcher.registerMethod<MemberAssignmentNode>(&ObjectEvaluator::evaluateMemberAssignmentNode);
        dispatcher.registerMethod<IndexAssignmentNode>(&ObjectEvaluator::evaluateIndexAssignmentNode);
    }

    bool ObjectEvaluator::canHandle(ASTNode* node) const
    {
        return isObjectNode(node);
    }


    void ObjectEvaluator::setExpressionEvaluator(ExpressionEvaluator* evaluator)
    {
        exprEvaluator = evaluator;
        arrayAssignmentHandler->setExpressionEvaluator(evaluator);
        classRegistrationHandler->setExpressionEvaluator(evaluator);
        genericInstantiationHandler->setExpressionEvaluator(evaluator);
    }

    void ObjectEvaluator::setStatementEvaluator(StatementEvaluator* evaluator)
    {
        stmtEvaluator = evaluator;
        staticMemberHandler->setStatementEvaluator(evaluator);
        instanceOperationHandler->setStatementEvaluator(evaluator);
        genericInstantiationHandler->setStatementEvaluator(evaluator);
    }

    bool ObjectEvaluator::isObjectNode(ASTNode* node) const
    {
        // Use NodeTypeRegistry for O(1) lookup instead of O(n) dynamic_cast chain
        return utils::NodeTypeRegistry::isObject(node);
    }

    Value ObjectEvaluator::evaluateClassNode(ClassNode* node)
    {
        return classRegistrationHandler->evaluateClass(node);
    }

    Value ObjectEvaluator::evaluateInterfaceNode(InterfaceNode* node)
    {
        return classRegistrationHandler->evaluateInterface(node);
    }

    void ObjectEvaluator::registerClass(std::shared_ptr<ClassDefinition> classDef)
    {
        classRegistrationHandler->registerClass(classDef);
    }

    Value ObjectEvaluator::evaluateNewNode(NewNode* node)
    {
        return genericInstantiationHandler->evaluateNew(node);
    }

    std::shared_ptr<ObjectInstance> ObjectEvaluator::createInstance(
        const std::string& className,
        const std::vector<Value>& constructorArgs)
    {
        return instanceOperationHandler->createInstance(className, constructorArgs);
    }

    std::shared_ptr<ObjectInstance> ObjectEvaluator::createInstanceWithTypeBindings(
        const std::string& className,
        const std::vector<Value>& constructorArgs,
        const std::unordered_map<std::string, std::string>& typeBindings)
    {
        return instanceOperationHandler->createInstanceWithTypeBindings(className, constructorArgs, typeBindings);
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
            return accessMember(instance, node->getMemberName(), node->getLocation());
        }
        // Handle all array types for .length property
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
        else if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(objectValue))
        {
            auto array = std::get<std::shared_ptr<value::FlatMultiArray>>(objectValue);
            if (node->getMemberName() == "length")
            {
                return static_cast<int>(array->getDimensions()[0]);
            }
            else
            {
                throw TypeException("Array does not have member '" + node->getMemberName() + "'");
            }
        }
        else if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(objectValue))
        {
            auto array = std::get<std::shared_ptr<value::SparseMultiArray>>(objectValue);
            if (node->getMemberName() == "length")
            {
                return static_cast<int>(array->getDimensions()[0]);
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
                                        const std::string& memberName,
                                        const SourceLocation& location)
    {
        return instanceOperationHandler->accessMember(object, memberName, location);
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
            assignMember(instance, node->getMemberName(), newValue, node->getLocation());
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
        return arrayAssignmentHandler->evaluateIndexAssignment(node);
    }

    void ObjectEvaluator::assignMember(std::shared_ptr<ObjectInstance> object,
                                       const std::string& memberName,
                                       const Value& value,
                                       const SourceLocation& location)
    {
        instanceOperationHandler->assignMember(object, memberName, value, location);
    }

    Value ObjectEvaluator::evaluateMethodCallNode(MethodCallNode* node)
    {
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for method call");
        }

        // Handle static method calls BEFORE evaluating arguments
        if (node->getIsStaticCall())
        {
            // For static calls, the object should be a VariableNode containing the class name
            if (auto varNode = dynamic_cast<nodes::expressions::VariableNode*>(node->getObject()))
            {
                std::string className = varNode->getName();

                // Evaluate arguments after confirming it's a valid static call
                std::vector<Value> args = evaluateArgumentList(node->getArguments());

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

        // Evaluate arguments first (needed for instance calls)
        std::vector<Value> args = evaluateArgumentList(node->getArguments());

        // Handle instance method calls
        Value objectValue;
        if (dynamic_cast<NewNode*>(node->getObject()))
        {
            // Handle NewNode directly since ExpressionEvaluator would delegate back to us anyway
            objectValue = evaluate(node->getObject());
        }
        else
        {
            // Delegate to expression evaluator for other nodes (like VariableNode, ArrayAccessNode, etc.)
            objectValue = exprEvaluator->evaluate(node->getObject());
        }

        // Handle direct lambda values (e.g., from array elements)
        if (std::holds_alternative<std::shared_ptr<value::LambdaValue>>(objectValue))
        {
            auto lambda = std::get<std::shared_ptr<value::LambdaValue>>(objectValue);
            if (!lambda) {
                throw TypeException("Lambda value is null - cannot invoke method '" +
                                  node->getMethodName() + "'", node->getLocation());
            }
            // Invoke the lambda with the method arguments
            return lambda->invoke(args, context);
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
                "' on non-object value", node->getLocation());
        }
    }

    Value ObjectEvaluator::callMethod(std::shared_ptr<ObjectInstance> object,
                                      const std::string& methodName,
                                      const std::vector<Value>& args,
                                      const errors::SourceLocation& location)
    {
        return instanceOperationHandler->callMethod(object, methodName, args, location);
    }

    std::vector<Value> ObjectEvaluator::evaluateArgumentList(
        const std::vector<std::unique_ptr<ASTNode>>& args)
    {
        return utils::ArgumentEvaluator::evaluateArguments(args, exprEvaluator);
    }

    // Static member operations (delegating to instance manager)
    Value ObjectEvaluator::accessStaticMember(const std::string& className,
                                              const std::string& memberName)
    {
        return staticMemberHandler->accessStaticMember(className, memberName);
    }

    void ObjectEvaluator::assignStaticMember(const std::string& className,
                                             const std::string& memberName,
                                             const Value& value)
    {
        staticMemberHandler->assignStaticMember(className, memberName, value);
    }

    Value ObjectEvaluator::callStaticMethod(const std::string& className,
                                            const std::string& methodName,
                                            const std::vector<Value>& args,
                                            const errors::SourceLocation& location)
    {
        return staticMemberHandler->callStaticMethod(className, methodName, args, location);
    }

    Value ObjectEvaluator::callStaticMethod(const std::string& className,
                                            const std::string& methodName,
                                            const std::vector<Value>& args,
                                            const std::vector<std::string>& genericTypeArguments,
                                            const errors::SourceLocation& location)
    {
        return staticMemberHandler->callStaticMethodWithGenerics(className, methodName, args, genericTypeArguments, location);
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


}
