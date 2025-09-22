#include "ObjectEvaluator.hpp"
#include "utils/ParameterBinder.hpp"
#include "utils/ScopeGuard.hpp"
#include "utils/GenericTypeManager.hpp"
#include "utils/ValueConverter.hpp"
#include "../value/FlatMultiArray.hpp"
#include "../value/SparseMultiArray.hpp"
#include "../ast/nodes/expressions/IndexAccessNode.hpp"
#include <iostream>
#include "../errors/TypeException.hpp"
#include "../errors/UndefinedException.hpp"
#include "../exception/ReturnException.hpp"
#include "../environment/manager/Scope.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../ast/nodes/classes/FieldNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../ast/nodes/classes/ConstructorNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
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
// All collection includes removed - collections now implemented in mType language
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
        auto classDef = std::make_shared<ClassDefinition>(node->getClassName(), node->getGenericParameters());

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

            if (methodNode->isGeneric()) {
                // For generic methods, preserve the generic type information
                methodDef = std::make_shared<MethodDefinition>(
                    methodNode->getName(),
                    methodNode->getReturnType(),               // Legacy ValueType for compatibility
                    methodNode->getParameters(),               // Legacy ValueType parameters for compatibility
                    std::vector<std::pair<std::string, Value>>{}, // empty arguments
                    bodyPtr,
                    methodNode->getIsStatic(),
                    methodNode->getGenericReturnType(),        // NEW: Preserve generic return type
                    methodNode->getGenericParameters(),        // NEW: Preserve generic method parameters
                    methodNode->getGenericTypeParameters(),    // NEW: Preserve generic type parameter declarations
                    std::unordered_map<std::string, std::string>{} // Empty substitution map for template
                );
            } else {
                // For non-generic methods, use legacy constructor
                methodDef = std::make_shared<MethodDefinition>(
                    methodNode->getName(),
                    methodNode->getReturnType(),
                    methodNode->getParameters(),
                    std::vector<std::pair<std::string, Value>>{}, // empty arguments
                    bodyPtr,
                    methodNode->getIsStatic()
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

        // Register class
        registerClass(classDef);

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

        // REMOVED: Collection handling - collections now implemented in mType language
        // All collections (Array, Map, Set, Stack, Queue) will be handled as regular mType classes

        /* if (className.find("Array<") == 0)
        {
            // Parse the element type from Array<ElementType>
            size_t start = className.find('<') + 1;
            size_t end = className.rfind('>'); // Use rfind to get the last '>' to handle nested generics
            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                std::string elementTypeName = className.substr(start, end - start);
                
                // Check if element type is a collection (nested generic)
                if (elementTypeName.find("Array<") == 0 || 
                    elementTypeName.find("Map<") == 0 || elementTypeName.find("Set<") == 0 ||
                    elementTypeName.find("Queue<") == 0 || elementTypeName.find("Stack<") == 0)
                {
                    // For nested collections, use OBJECT type and store the full type name
                    return std::make_shared<runtimeTypes::collections::Array>(value::ValueType::OBJECT, elementTypeName);
                }
                else
                {
                    // For simple types, use the existing logic
                    value::ValueType elementType = parser::TypeParser::stringToValueType(elementTypeName);
                    
                    // If elementType is OBJECT, it means it's a class name - store the class name for validation
                    if (elementType == value::ValueType::OBJECT) {
                        return std::make_shared<runtimeTypes::collections::Array>(elementType, elementTypeName);
                    } else {
                        return std::make_shared<runtimeTypes::collections::Array>(elementType);
                    }
                }
            }
            else
            {
                // Fallback to default element type
                return std::make_shared<runtimeTypes::collections::Array>(value::ValueType::OBJECT);
            }
        }
        else if (className.find("Map<") == 0)
        {
            // Parse the key and value types from Map<KeyType, ValueType>
            size_t start = className.find('<') + 1;
            size_t end = className.find('>');
            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                std::string typeParams = className.substr(start, end - start);
                size_t commaPos = typeParams.find(',');
                if (commaPos != std::string::npos)
                {
                    std::string keyTypeName = typeParams.substr(0, commaPos);
                    std::string valueTypeName = typeParams.substr(commaPos + 1);
                    // Remove leading/trailing spaces
                    keyTypeName.erase(0, keyTypeName.find_first_not_of(' '));
                    keyTypeName.erase(keyTypeName.find_last_not_of(' ') + 1);
                    valueTypeName.erase(0, valueTypeName.find_first_not_of(' '));
                    valueTypeName.erase(valueTypeName.find_last_not_of(' ') + 1);
                    
                    // Check if key type is a collection (nested generic)
                    value::ValueType keyType;
                    std::string keyClassName = "";
                    if (keyTypeName.find("Array<") == 0 || 
                        keyTypeName.find("Map<") == 0 || keyTypeName.find("Set<") == 0 ||
                        keyTypeName.find("Queue<") == 0 || keyTypeName.find("Stack<") == 0) {
                        keyType = value::ValueType::OBJECT;
                        keyClassName = keyTypeName;
                    } else {
                        keyType = parser::TypeParser::stringToValueType(keyTypeName);
                        if (keyType == value::ValueType::OBJECT) {
                            keyClassName = keyTypeName;
                        }
                    }
                    
                    // Check if value type is a collection (nested generic)
                    value::ValueType valueType;
                    std::string valueClassName = "";
                    if (valueTypeName.find("Array<") == 0 || 
                        valueTypeName.find("Map<") == 0 || valueTypeName.find("Set<") == 0 ||
                        valueTypeName.find("Queue<") == 0 || valueTypeName.find("Stack<") == 0) {
                        valueType = value::ValueType::OBJECT;
                        valueClassName = valueTypeName;
                    } else {
                        valueType = parser::TypeParser::stringToValueType(valueTypeName);
                        if (valueType == value::ValueType::OBJECT) {
                            valueClassName = valueTypeName;
                        }
                    }
                    
                    // Create Map with appropriate class name (currently only supports value type class names)
                    if (!valueClassName.empty()) {
                        return std::make_shared<runtimeTypes::collections::Map>(keyType, valueType, valueClassName);
                    } else {
                        return std::make_shared<runtimeTypes::collections::Map>(keyType, valueType);
                    }
                }
            }
            return std::make_shared<runtimeTypes::collections::Map>(value::ValueType::STRING, value::ValueType::OBJECT);
        }
        else if (className.find("Set<") == 0)
        {
            size_t start = className.find('<') + 1;
            size_t end = className.find('>');
            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                std::string elementTypeName = className.substr(start, end - start);
                value::ValueType elementType = parser::TypeParser::stringToValueType(elementTypeName);
                return std::make_shared<runtimeTypes::collections::Set>(elementType);
            }
            else
            {
                return std::make_shared<runtimeTypes::collections::Set>(value::ValueType::OBJECT);
            }
        }
        else if (className.find("Queue<") == 0)
        {
            size_t start = className.find('<') + 1;
            size_t end = className.find('>');
            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                std::string elementTypeName = className.substr(start, end - start);
                value::ValueType elementType = parser::TypeParser::stringToValueType(elementTypeName);
                return std::make_shared<runtimeTypes::collections::Queue>(elementType);
            }
            else
            {
                return std::make_shared<runtimeTypes::collections::Queue>(value::ValueType::OBJECT);
            }
        }
        else if (className.find("Stack<") == 0)
        {
            size_t start = className.find('<') + 1;
            size_t end = className.find('>');
            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                std::string elementTypeName = className.substr(start, end - start);
                value::ValueType elementType = parser::TypeParser::stringToValueType(elementTypeName);
                return std::make_shared<runtimeTypes::collections::Stack>(elementType);
            }
            else
            {
                return std::make_shared<runtimeTypes::collections::Stack>(value::ValueType::OBJECT);
            }
        } */

        // Handle regular class instantiation
        // className already declared above for collections
        std::shared_ptr<ClassDefinition> classDef;
        auto env = context->getEnvironment();

        // Try to resolve type parameters from current object context first
        std::string resolvedClassName = className;
        if (utils::GenericTypeManager::isGenericInstantiation(className)) {
            auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(className);

            std::vector<std::string> resolvedTypeArguments;
            bool hasResolvedParams = false;
            for (const std::string& typeArg : typeArguments) {
                std::string resolvedType = resolveTypeParameterFromContext(typeArg);
                resolvedTypeArguments.push_back(resolvedType);
                if (resolvedType != typeArg) {
                    hasResolvedParams = true;
                }
            }

            // If we resolved any type parameters, construct the resolved class name
            if (hasResolvedParams) {
                resolvedClassName = baseName + "<" + resolvedTypeArguments[0];
                for (size_t i = 1; i < resolvedTypeArguments.size(); ++i) {
                    resolvedClassName += "," + resolvedTypeArguments[i];
                }
                resolvedClassName += ">";
            }
        }

        // Check if we should use the resolved class name for direct instantiation
        if (resolvedClassName != className) {
            // We resolved type parameters - try to find the instantiated class directly
            auto instantiatedClass = env->findClass(resolvedClassName);
            if (instantiatedClass) {
                classDef = instantiatedClass;
            } else {
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
                throw TypeException("Class '" + baseName + "' is not generic but used with type arguments");
            }

            // Validate type arguments
            if (!utils::GenericTypeManager::validateTypeArguments(genericClass, typeArguments))
            {
                throw TypeException("Invalid type arguments for generic class '" + baseName + "'");
            }

            // Create instantiated class
            classDef = utils::GenericTypeManager::instantiateGenericClass(genericClass, typeArguments);
            if (!classDef)
            {
                throw TypeException("Failed to instantiate generic class '" + className + "'");
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
        auto instance = createInstance(classNameForInstance, args);

        // Execute constructor if it exists
        if (classDef && !classDef->getConstructors().empty())
        {
            auto constructor = classDef->findConstructor(args.size());
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
                            env
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
        if (!object) {
            throw TypeException("Cannot access member '" + memberName + "' on null object");
        }

        // VALIDATION: Prevent instance member access from static methods
        if (context->isInStaticMethodContext()) {
            auto field = object->getField(memberName);
            if (field && !field->isStatic()) {
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
        if (multiDimResult.has_value()) {
            auto [baseArray, indices] = multiDimResult.value();
            Value newValue = exprEvaluator->evaluate(node->getValue());
            return performDirectMultiDimensionalAssignment(baseArray, indices, newValue, node->getLocation());
        }

        // Check if this is a multi-dimensional assignment (e.g., arr2d[0][0] = value)
        // If node->getObject() is an IndexAccessNode, we might have a multi-dimensional assignment
        auto objectASTNode = node->getObject();

        // Try to detect if this is an IndexAccessNode
        if (auto indexAccessNode = dynamic_cast<ast::nodes::expressions::IndexAccessNode*>(objectASTNode)) {
            // Get the base array (e.g., arr2d in arr2d[0][0] = value)
            Value baseArrayValue = exprEvaluator->evaluate(indexAccessNode->getCollection());

            if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(baseArrayValue)) {
                auto baseArray = std::get<std::shared_ptr<value::FlatMultiArray>>(baseArrayValue);

                // Get the first dimension index (e.g., 0 in arr2d[0][0])
                Value firstIndexValue = exprEvaluator->evaluate(indexAccessNode->getIndex());

                // Get the second dimension index (e.g., 0 in arr2d[0][0])
                Value secondIndexValue = exprEvaluator->evaluate(node->getIndex());

                // Get the new value
                Value newValue = exprEvaluator->evaluate(node->getValue());

                if (std::holds_alternative<int>(firstIndexValue) && std::holds_alternative<int>(secondIndexValue)) {
                    int firstIndex = std::get<int>(firstIndexValue);
                    int secondIndex = std::get<int>(secondIndexValue);

                    // Validate indices before conversion
                    if (firstIndex < 0) {
                        throw TypeException("Multi-dimensional array first index " + std::to_string(firstIndex) + " is negative", node->getLocation());
                    }
                    if (secondIndex < 0) {
                        throw TypeException("Multi-dimensional array second index " + std::to_string(secondIndex) + " is negative", node->getLocation());
                    }

                    std::vector<size_t> indices;
                    indices.push_back(static_cast<size_t>(firstIndex));
                    indices.push_back(static_cast<size_t>(secondIndex));

                    try {
                        baseArray->set(indices, newValue);
                        return newValue;
                    } catch (const std::out_of_range& e) {
                        throw TypeException("Multi-dimensional array assignment failed: " + std::string(e.what()), node->getLocation());
                    }
                }
            } else if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(baseArrayValue)) {
                auto baseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(baseArrayValue);

                // Get the first dimension index (e.g., 0 in arr2d[0][0])
                Value firstIndexValue = exprEvaluator->evaluate(indexAccessNode->getIndex());

                // Get the second dimension index (e.g., 0 in arr2d[0][0])
                Value secondIndexValue = exprEvaluator->evaluate(node->getIndex());

                // Get the new value
                Value newValue = exprEvaluator->evaluate(node->getValue());

                if (std::holds_alternative<int>(firstIndexValue) && std::holds_alternative<int>(secondIndexValue)) {
                    int firstIndex = std::get<int>(firstIndexValue);
                    int secondIndex = std::get<int>(secondIndexValue);

                    // Validate indices before conversion
                    if (firstIndex < 0) {
                        throw TypeException("Sparse multi-dimensional array first index " + std::to_string(firstIndex) + " is negative", node->getLocation());
                    }
                    if (secondIndex < 0) {
                        throw TypeException("Sparse multi-dimensional array second index " + std::to_string(secondIndex) + " is negative", node->getLocation());
                    }

                    std::vector<size_t> indices;
                    indices.push_back(static_cast<size_t>(firstIndex));
                    indices.push_back(static_cast<size_t>(secondIndex));

                    try {
                        baseArray->set(indices, newValue);
                        return newValue;
                    } catch (const std::out_of_range& e) {
                        throw TypeException("Sparse multi-dimensional array assignment failed: " + std::string(e.what()), node->getLocation());
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


        // Check if index is an integer
        if (!std::holds_alternative<int>(indexValue)) {
            throw TypeException("Array index must be an integer", node->getLocation());
        }

        int index = std::get<int>(indexValue);

        // Check if object is a NativeArray
        if (std::holds_alternative<std::shared_ptr<value::NativeArray>>(objectValue)) {
            auto nativeArray = std::get<std::shared_ptr<value::NativeArray>>(objectValue);

            // Check bounds with descriptive error message
            if (index < 0) {
                throw TypeException("Array assignment index " + std::to_string(index) + " is negative (valid range: 0 to " +
                                  std::to_string(nativeArray->size() - 1) + ")", node->getLocation());
            }
            if (static_cast<size_t>(index) >= nativeArray->size()) {
                throw TypeException("Array assignment index " + std::to_string(index) + " exceeds array size of " +
                                  std::to_string(nativeArray->size()) + " elements (valid range: 0 to " +
                                  std::to_string(nativeArray->size() - 1) + ")", node->getLocation());
            }

            nativeArray->set(static_cast<size_t>(index), newValue);
            return newValue;
        }

        // Check if object is a FlatMultiArray (for multi-dimensional arrays)
        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(objectValue)) {
            auto flatArray = std::get<std::shared_ptr<value::FlatMultiArray>>(objectValue);

            // Check bounds with descriptive error message
            if (index < 0) {
                throw TypeException("Multi-dimensional array assignment index " + std::to_string(index) + " is negative (valid range: 0 to " +
                                  std::to_string(flatArray->size() - 1) + ")", node->getLocation());
            }
            if (static_cast<size_t>(index) >= flatArray->size()) {
                throw TypeException("Multi-dimensional array assignment index " + std::to_string(index) + " exceeds array size of " +
                                  std::to_string(flatArray->size()) + " elements (valid range: 0 to " +
                                  std::to_string(flatArray->size() - 1) + ")", node->getLocation());
            }

            // For 1D FlatMultiArray, set directly
            if (flatArray->getRank() == 1) {
                try {
                    flatArray->set(static_cast<size_t>(index), newValue);
                    return newValue;
                } catch (const std::out_of_range& e) {
                    throw TypeException("Array assignment failed: " + std::string(e.what()), node->getLocation());
                }
            } else {
                // For multi-dimensional arrays, this should be handled differently
                // The parser should create nested IndexAssignmentNodes, but if we get here,
                // it means we're assigning to a sub-array which isn't supported
                throw TypeException("Cannot assign array to index position", node->getLocation());
            }
        }

        // Check if object is a SparseMultiArray (for adaptive sparse arrays)
        if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(objectValue)) {
            auto sparseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(objectValue);

            // Check bounds with descriptive error message
            if (index < 0) {
                throw TypeException("Sparse array assignment index " + std::to_string(index) + " is negative (valid range: 0 to " +
                                  std::to_string(sparseArray->size() - 1) + ")", node->getLocation());
            }
            if (static_cast<size_t>(index) >= sparseArray->size()) {
                throw TypeException("Sparse array assignment index " + std::to_string(index) + " exceeds array size of " +
                                  std::to_string(sparseArray->size()) + " elements (valid range: 0 to " +
                                  std::to_string(sparseArray->size() - 1) + ")", node->getLocation());
            }

            // For single dimension sparse array
            if (sparseArray->getRank() == 1) {
                try {
                    std::vector<size_t> indices = {static_cast<size_t>(index)};
                    sparseArray->set(indices, newValue);
                    return newValue;
                } catch (const std::out_of_range& e) {
                    throw TypeException("Sparse array assignment failed: " + std::string(e.what()), node->getLocation());
                }
            } else {
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
                std::cout << "[DEBUG] ERROR: Index is not an integer" << std::endl;
                throw TypeException("Array index must be an integer");
            }
        }
        else
        {
            std::cout << "[DEBUG] ERROR: Object is not an Array" << std::endl;
            throw TypeException("Cannot assign to index on non-array value");
        } */
    }

    void ObjectEvaluator::assignMember(std::shared_ptr<ObjectInstance> object,
                                       const std::string& memberName,
                                       const Value& value)
    {
        if (!object) {
            throw TypeException("Cannot assign to member '" + memberName + "' on null object");
        }

        // VALIDATION: Prevent instance member assignment from static methods
        if (context->isInStaticMethodContext()) {
            auto field = object->getField(memberName);
            if (field && !field->isStatic()) {
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
                                          node->getGenericTypeArguments());
                }
                else
                {
                    return callStaticMethod(className, node->getMethodName(), args);
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
            return callMethod(instance, node->getMethodName(), args);
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
                                      const std::vector<Value>& args)
    {
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
                if (method->hasGenericInformation()) {
                    // Use the new generic-aware parameter binding
                    utils::ParameterBinder::bindAndValidateParameters(
                        method,
                        args,
                        "method '" + methodName + "'",
                        env
                    );
                } else {
                    // Use legacy parameter binding for non-generic methods
                    utils::ParameterBinder::bindAndValidateParameters(
                        method->getParameters(),
                        args,
                        "method '" + methodName + "'",
                        env
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
                return result;
            }
            catch (const exception::ReturnException& e)
            {
                // Handle return exception - extract return value
                context->setInStaticMethod(wasInStaticMethod); // Restore static method context
                context->setCurrentInstance(prevInstance);
                context->setReturned(false); // Reset return state after handling exception
                return e.returnValue;
            }
            catch (...)
            {
                context->setInStaticMethod(wasInStaticMethod); // Restore static method context
                context->setCurrentInstance(prevInstance);
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
                                            const std::vector<Value>& args)
    {
        auto env = context->getEnvironment();

        // Declare previousMethod for restoration in exception handlers
        auto previousMethod = context->getCurrentMethod();

        // Try to resolve type parameters from current object context first
        std::string resolvedClassName = className;
        auto currentInstance = context->getCurrentInstance();
        if (currentInstance) {
            auto instanceClassDef = currentInstance->getClassDefinition();
            if (instanceClassDef) {
                std::string instanceClassName = instanceClassDef->getName(); // e.g., "Set<int>"

                // Check if the target className contains type parameters that need resolution
                if (className.find('<') != std::string::npos && className.find('T') != std::string::npos) {
                    if (utils::GenericTypeManager::isGenericInstantiation(instanceClassName)) {
                        auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(instanceClassName);

                        // Replace type parameters in className
                        resolvedClassName = className;
                        if (className.find("T") != std::string::npos && !typeArguments.empty()) {
                            // Simple T replacement - can be extended for multiple type parameters
                            size_t pos = resolvedClassName.find("T");
                            while (pos != std::string::npos) {
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
                    env
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
                                            const std::vector<std::string>& genericTypeArguments)
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
                throw TypeException("Method '" + methodName + "' is not generic but generic type arguments were provided");
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
            static std::unordered_map<std::string, std::shared_ptr<runtimeTypes::klass::MethodDefinition>>
                staticGenericMethodCache;

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

                // Cache the instantiated method
                staticGenericMethodCache[signatureKey] = methodToCall;
            }
        }

        // Use ScopeGuard and ParameterBinder utilities
        {
            utils::ScopeGuard scope(env, methodName, environment::manager::ScopeType::FUNCTION);

            try
            {
                // Use ParameterBinder utility for consistent parameter validation and binding
                utils::ParameterBinder::bindAndValidateParameters(
                    methodToCall->getParameters(),
                    args,
                    "static method '" + className + "::" + methodName + "'",
                    env
                );

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
        // TODO: Implement method definition evaluation
        throw TypeException("Method definition evaluation not implemented in refactored version");
    }

    Value ObjectEvaluator::evaluateFieldNode(FieldNode* node)
    {
        // TODO: Implement field definition evaluation
        throw TypeException("Field definition evaluation not implemented in refactored version");
    }

    Value ObjectEvaluator::evaluateConstructorNode(ConstructorNode* node)
    {
        // TODO: Implement constructor definition evaluation
        throw TypeException("Constructor definition evaluation not implemented in refactored version");
    }
    
    
    // REMOVED: Collection method dispatch - collections now implemented in mType language
    /* Value ObjectEvaluator::dispatchCollectionMethod(const Value& collectionValue,
                                                   const std::string& methodName,
                                                   const std::vector<Value>& args)
    {
        // This method is no longer needed since collections are implemented in mType
        throw TypeException("Collection method dispatch removed - collections now handled as mType objects");
    } */

    // REMOVED: All collection method implementations - collections now implemented in mType language
    /* Value ObjectEvaluator::callArrayMethod(std::shared_ptr<runtimeTypes::collections::Array> array,
                                          const std::string& methodName,
                                          const std::vector<Value>& args)
    {
        // Common collection methods
        if (methodName == "size")
        {
            if (!args.empty())
                throw TypeException("size() method takes no arguments");
            return static_cast<int>(array->size());
        }
        else if (methodName == "empty")
        {
            if (!args.empty())
                throw TypeException("empty() method takes no arguments");
            return array->empty();
        }
        else if (methodName == "clear")
        {
            if (!args.empty())
                throw TypeException("clear() method takes no arguments");
            array->clear();
            return std::monostate{};
        }
        // Array-specific methods
        else if (methodName == "get")
        {
            if (args.size() != 1)
                throw TypeException("get() method takes exactly 1 argument");
            if (!std::holds_alternative<int>(args[0]))
                throw TypeException("Array index must be an integer");
            int index = std::get<int>(args[0]);
            if (index < 0)
                throw TypeException("Array index cannot be negative");
            return array->get(static_cast<size_t>(index));
        }
        else if (methodName == "set")
        {
            if (args.size() != 2)
                throw TypeException("set() method takes exactly 2 arguments");
            if (!std::holds_alternative<int>(args[0]))
                throw TypeException("Array index must be an integer");
            int index = std::get<int>(args[0]);
            if (index < 0)
                throw TypeException("Array index cannot be negative");
            array->set(static_cast<size_t>(index), args[1]);
            return std::monostate{};
        }
        else if (methodName == "add" || methodName == "push")
        {
            if (args.size() != 1)
                throw TypeException("add() method takes exactly 1 argument");
            array->add(args[0]);
            return std::monostate{};
        }
        else if (methodName == "removeAt")
        {
            if (args.size() != 1)
                throw TypeException("removeAt() method takes exactly 1 argument");
            if (!std::holds_alternative<int>(args[0]))
                throw TypeException("Array index must be an integer");
            int index = std::get<int>(args[0]);
            if (index < 0)
                throw TypeException("Array index cannot be negative");
            array->removeAt(static_cast<size_t>(index));
            return std::monostate{};
        }
        else
        {
            throw TypeException("Unknown method '" + methodName + "' for Array type");
        }
    }

    // Specialized Map method operations
    Value ObjectEvaluator::callMapMethod(std::shared_ptr<runtimeTypes::collections::Map> map,
                                        const std::string& methodName,
                                        const std::vector<Value>& args)
    {
        // Common collection methods
        if (methodName == "size")
        {
            if (!args.empty())
                throw TypeException("size() method takes no arguments");
            return static_cast<int>(map->size());
        }
        else if (methodName == "empty")
        {
            if (!args.empty())
                throw TypeException("empty() method takes no arguments");
            return map->empty();
        }
        else if (methodName == "clear")
        {
            if (!args.empty())
                throw TypeException("clear() method takes no arguments");
            map->clear();
            return std::monostate{};
        }
        // Map-specific methods
        else if (methodName == "get")
        {
            if (args.size() != 1)
                throw TypeException("get() method takes exactly 1 argument");
            std::string key = exprEvaluator->toString(args[0]);
            return map->get(key);
        }
        else if (methodName == "put")
        {
            if (args.size() != 2)
                throw TypeException("put() method takes exactly 2 arguments");
            std::string key = exprEvaluator->toString(args[0]);
            map->put(key, args[1]);
            return std::monostate{};
        }
        else if (methodName == "containsKey")
        {
            if (args.size() != 1)
                throw TypeException("containsKey() method takes exactly 1 argument");
            std::string key = exprEvaluator->toString(args[0]);
            return map->containsKey(key);
        }
        else if (methodName == "keySet")
        {
            if (!args.empty())
                throw TypeException("keySet() method takes no arguments");
            return map->keySet();
        }
        else if (methodName == "remove")
        {
            if (args.size() != 1)
                throw TypeException("remove() method takes exactly 1 argument");
            std::string key = exprEvaluator->toString(args[0]);
            map->remove(key);
            return std::monostate{};
        }
        else
        {
            throw TypeException("Unknown method '" + methodName + "' for Map type");
        }
    }

    // Specialized Set method operations
    Value ObjectEvaluator::callSetMethod(std::shared_ptr<runtimeTypes::collections::Set> set,
                                        const std::string& methodName,
                                        const std::vector<Value>& args)
    {
        // Common collection methods
        if (methodName == "size")
        {
            if (!args.empty())
                throw TypeException("size() method takes no arguments");
            return static_cast<int>(set->size());
        }
        else if (methodName == "empty")
        {
            if (!args.empty())
                throw TypeException("empty() method takes no arguments");
            return set->empty();
        }
        else if (methodName == "clear")
        {
            if (!args.empty())
                throw TypeException("clear() method takes no arguments");
            set->clear();
            return std::monostate{};
        }
        // Set-specific methods
        else if (methodName == "add")
        {
            if (args.size() != 1)
                throw TypeException("add() method takes exactly 1 argument");
            bool added = set->add(args[0]);
            return added;
        }
        else if (methodName == "contains")
        {
            if (args.size() != 1)
                throw TypeException("contains() method takes exactly 1 argument");
            return set->contains(args[0]);
        }
        else if (methodName == "remove")
        {
            if (args.size() != 1)
                throw TypeException("remove() method takes exactly 1 argument");
            bool removed = set->remove(args[0]);
            return removed;
        }
        else
        {
            throw TypeException("Unknown method '" + methodName + "' for Set type");
        }
    }

    // Specialized Stack method operations
    Value ObjectEvaluator::callStackMethod(std::shared_ptr<runtimeTypes::collections::Stack> stack,
                                          const std::string& methodName,
                                          const std::vector<Value>& args)
    {
        // Common collection methods
        if (methodName == "size")
        {
            if (!args.empty())
                throw TypeException("size() method takes no arguments");
            return static_cast<int>(stack->size());
        }
        else if (methodName == "empty")
        {
            if (!args.empty())
                throw TypeException("empty() method takes no arguments");
            return stack->empty();
        }
        else if (methodName == "clear")
        {
            if (!args.empty())
                throw TypeException("clear() method takes no arguments");
            stack->clear();
            return std::monostate{};
        }
        // Stack-specific methods
        else if (methodName == "push")
        {
            if (args.size() != 1)
                throw TypeException("push() method takes exactly 1 argument");
            stack->push(args[0]);
            return std::monostate{};
        }
        else if (methodName == "pop")
        {
            if (!args.empty())
                throw TypeException("pop() method takes no arguments");
            return stack->pop();
        }
        else if (methodName == "top")
        {
            if (!args.empty())
                throw TypeException("top() method takes no arguments");
            return stack->top();
        }
        else
        {
            throw TypeException("Unknown method '" + methodName + "' for Stack type");
        }
    }

    // Specialized Queue method operations
    Value ObjectEvaluator::callQueueMethod(std::shared_ptr<runtimeTypes::collections::Queue> queue,
                                          const std::string& methodName,
                                          const std::vector<Value>& args)
    {
        // Common collection methods
        if (methodName == "size")
        {
            if (!args.empty())
                throw TypeException("size() method takes no arguments");
            return static_cast<int>(queue->size());
        }
        else if (methodName == "empty")
        {
            if (!args.empty())
                throw TypeException("empty() method takes no arguments");
            return queue->empty();
        }
        else if (methodName == "clear")
        {
            if (!args.empty())
                throw TypeException("clear() method takes no arguments");
            queue->clear();
            return std::monostate{};
        }
        // Queue-specific methods
        else if (methodName == "enqueue")
        {
            if (args.size() != 1)
                throw TypeException("enqueue() method takes exactly 1 argument");
            queue->enqueue(args[0]);
            return std::monostate{};
        }
        else if (methodName == "dequeue")
        {
            if (!args.empty())
                throw TypeException("dequeue() method takes no arguments");
            return queue->dequeue();
        }
        else if (methodName == "front")
        {
            if (!args.empty())
                throw TypeException("front() method takes no arguments");
            return queue->front();
        }
        else
        {
            throw TypeException("Unknown method '" + methodName + "' for Queue type");
        }
    } */



    std::string ObjectEvaluator::resolveTypeParameterFromContext(const std::string& typeParam)
    {
        // First try to resolve from current method context (for static generic methods)
        auto currentMethod = context->getCurrentMethod();
        if (currentMethod && currentMethod->hasGenericInformation()) {
            const auto& substitutionMap = currentMethod->getTypeSubstitutionMap();
            auto it = substitutionMap.find(typeParam);
            if (it != substitutionMap.end()) {
                return it->second;
            }
        }

        // Try to resolve type parameters from the current object instance context
        auto currentInstance = context->getCurrentInstance();
        if (!currentInstance) {
            return typeParam;
        }

        auto classDef = currentInstance->getClassDefinition();
        if (!classDef) {
            return typeParam;
        }

        // Get the class name which should contain the instantiated type
        // e.g., "Set<int>" for a Set<int> instance
        std::string className = classDef->getName();

        // Check if the class name itself is a generic instantiation (e.g., "Set<int>")
        // This handles the case where we have an instantiated generic class
        if (!classDef->isGeneric() && !utils::GenericTypeManager::isGenericInstantiation(className)) {
            return typeParam;
        }

        // Parse the generic instantiation to extract type parameters
        if (utils::GenericTypeManager::isGenericInstantiation(className)) {
            auto [baseName, typeArguments] = utils::GenericTypeManager::parseGenericInstantiation(className);

            // Get the generic class definition to find its type parameter names
            auto env = context->getEnvironment();
            auto genericClass = env->findClass(baseName);
            if (genericClass && genericClass->isGeneric()) {
                auto genericParams = genericClass->getGenericParameters();

                // Handle array types (T[], T[][], Element[], etc.)
                if (typeParam.find("[]") != std::string::npos) {
                    std::string baseType = typeParam.substr(0, typeParam.find("[]"));
                    std::string arraySuffix = typeParam.substr(typeParam.find("[]"));

                    // Find the position of baseType in the generic parameters
                    for (size_t i = 0; i < genericParams.size() && i < typeArguments.size(); ++i) {
                        if (genericParams[i].name == baseType) {
                            return typeArguments[i] + arraySuffix;
                        }
                    }
                }
                // For simple cases like Set<T>, map any type parameter to its corresponding argument
                else {
                    // Find the position of typeParam in the generic parameters
                    for (size_t i = 0; i < genericParams.size() && i < typeArguments.size(); ++i) {
                        if (genericParams[i].name == typeParam) {
                            return typeArguments[i];
                        }
                    }
                }
            }
        }

        // If we can't resolve, return the original parameter
        return typeParam;
    }

    std::optional<std::pair<Value, std::vector<size_t>>> ObjectEvaluator::extractMultiDimensionalAssignment(IndexAssignmentNode* node)
    {
        auto objectASTNode = node->getObject();

        // Check if this is a 2D assignment: arr[i][j] = value
        if (auto indexAccessNode = dynamic_cast<ast::nodes::expressions::IndexAccessNode*>(objectASTNode)) {

            // Check if this is a 3D assignment: arr[i][j][k] = value
            // In this case, indexAccessNode->getCollection() would be another IndexAccessNode
            if (auto innerIndexAccess = dynamic_cast<ast::nodes::expressions::IndexAccessNode*>(indexAccessNode->getCollection())) {

                // This is a 3D assignment: arr[i][j][k] = value
                // Structure: IndexAssignmentNode(IndexAccessNode(IndexAccessNode(arr, i), j), k) = value

                Value baseArray = exprEvaluator->evaluate(innerIndexAccess->getCollection());

                // Check if it's a 3D array
                bool is3D = false;
                if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(baseArray)) {
                    auto flatArray = std::get<std::shared_ptr<value::FlatMultiArray>>(baseArray);
                    is3D = flatArray->getRank() == 3;
                } else if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(baseArray)) {
                    auto sparseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(baseArray);
                    is3D = sparseArray->getRank() == 3;
                }

                if (!is3D) {
                    return std::nullopt;
                }

                // Extract the three indices: [i][j][k]
                std::vector<size_t> indices;

                // First index (i): from innerIndexAccess->getIndex()
                Value firstIndexValue = exprEvaluator->evaluate(innerIndexAccess->getIndex());
                if (!std::holds_alternative<int>(firstIndexValue)) {
                    return std::nullopt;
                }
                int firstIndex = std::get<int>(firstIndexValue);
                if (firstIndex < 0) {
                    return std::nullopt;
                }

                // Second index (j): from indexAccessNode->getIndex()
                Value secondIndexValue = exprEvaluator->evaluate(indexAccessNode->getIndex());
                if (!std::holds_alternative<int>(secondIndexValue)) {
                    return std::nullopt;
                }
                int secondIndex = std::get<int>(secondIndexValue);
                if (secondIndex < 0) {
                    return std::nullopt;
                }

                // Third index (k): from node->getIndex()
                Value thirdIndexValue = exprEvaluator->evaluate(node->getIndex());
                if (!std::holds_alternative<int>(thirdIndexValue)) {
                    return std::nullopt;
                }
                int thirdIndex = std::get<int>(thirdIndexValue);
                if (thirdIndex < 0) {
                    return std::nullopt;
                }

                indices.push_back(static_cast<size_t>(firstIndex));
                indices.push_back(static_cast<size_t>(secondIndex));
                indices.push_back(static_cast<size_t>(thirdIndex));

                return std::make_pair(baseArray, indices);
            } else {
                // This is a 2D assignment: arr[i][j] = value
                // Let the existing 2D logic handle this
                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    Value ObjectEvaluator::performDirectMultiDimensionalAssignment(const Value& baseArray, const std::vector<size_t>& indices, const Value& newValue, const SourceLocation& location)
    {
        // Handle FlatMultiArray direct assignment
        if (std::holds_alternative<std::shared_ptr<value::FlatMultiArray>>(baseArray)) {
            auto flatArray = std::get<std::shared_ptr<value::FlatMultiArray>>(baseArray);

            try {
                flatArray->set(indices, newValue);
                return newValue;
            } catch (const std::out_of_range& e) {
                throw TypeException("Multi-dimensional array assignment failed: " + std::string(e.what()), location);
            }
        }

        // Handle SparseMultiArray direct assignment
        if (std::holds_alternative<std::shared_ptr<value::SparseMultiArray>>(baseArray)) {
            auto sparseArray = std::get<std::shared_ptr<value::SparseMultiArray>>(baseArray);

            try {
                sparseArray->set(indices, newValue);
                return newValue;
            } catch (const std::out_of_range& e) {
                throw TypeException("Sparse multi-dimensional array assignment failed: " + std::string(e.what()), location);
            }
        }

        throw TypeException("Unsupported array type for direct multi-dimensional assignment", location);
    }

    // Template instantiations not needed - template is now in header
}
