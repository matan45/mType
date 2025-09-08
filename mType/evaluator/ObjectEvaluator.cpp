#include "ObjectEvaluator.hpp"
#include "utils/ParameterBinder.hpp"
#include "utils/ScopeGuard.hpp"
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
#include "../ast/nodes/classes/MethodCallNode.hpp"
#include "../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/MethodDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../runtimeTypes/collections/Array.hpp"
#include "../runtimeTypes/collections/List.hpp"
#include "../runtimeTypes/collections/Map.hpp"
#include "../runtimeTypes/collections/Set.hpp"
#include "../runtimeTypes/collections/Queue.hpp"
#include "../runtimeTypes/collections/Stack.hpp"
#include "../parser/TypeParser.hpp"
#include "../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../runtimeTypes/collections/Array.hpp"
#include "../runtimeTypes/collections/Map.hpp"
#include "../runtimeTypes/collections/List.hpp"
#include "../runtimeTypes/collections/Set.hpp"
#include "ExpressionEvaluator.hpp"
#include "StatementEvaluator.hpp"

namespace evaluator
{
    using namespace errors;
    using namespace runtimeTypes::klass;

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
            dynamic_cast<MemberAssignmentNode*>(node);
    }

    Value ObjectEvaluator::evaluateClassNode(ClassNode* node)
    {
        auto env = context->getEnvironment();

        // Create class definition
        auto classDef = std::make_shared<ClassDefinition>(node->getClassName());

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

            auto methodDef = std::make_shared<MethodDefinition>(
                methodNode->getName(),
                methodNode->getReturnType(),
                methodNode->getParameters(),
                std::vector<std::pair<std::string, Value>>{}, // empty arguments
                bodyPtr,
                methodNode->getIsStatic()
            );

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
        
        // Handle collection types specially
        if (className.find("Array<") == 0)
        {
            // Parse the element type from Array<ElementType>
            size_t start = className.find('<') + 1;
            size_t end = className.rfind('>'); // Use rfind to get the last '>' to handle nested generics
            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                std::string elementTypeName = className.substr(start, end - start);
                
                // Check if element type is a collection (nested generic)
                if (elementTypeName.find("Array<") == 0 || elementTypeName.find("List<") == 0 || 
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
        else if (className.find("List<") == 0)
        {
            // Parse the element type from List<ElementType>
            size_t start = className.find('<') + 1;
            size_t end = className.find('>');
            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                std::string elementTypeName = className.substr(start, end - start);
                value::ValueType elementType = parser::TypeParser::stringToValueType(elementTypeName);
                return std::make_shared<runtimeTypes::collections::List>(elementType);
            }
            else
            {
                return std::make_shared<runtimeTypes::collections::List>(value::ValueType::OBJECT);
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
                    if (keyTypeName.find("Array<") == 0 || keyTypeName.find("List<") == 0 || 
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
                    if (valueTypeName.find("Array<") == 0 || valueTypeName.find("List<") == 0 || 
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
        }

        // Handle regular class instantiation
        auto instance = createInstance(node->getClassName(), args);

        // Execute constructor if it exists
        auto env = context->getEnvironment();
        auto classDef = env->findClass(node->getClassName());
        if (classDef && !classDef->getConstructors().empty())
        {
            auto constructor = classDef->findConstructor(args.size());
            if (constructor && constructor->getBody())
            {
                // Set the current instance for constructor execution
                auto prevInstance = context->getCurrentInstance();
                context->setCurrentInstance(instance);

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
                        throw;
                    }
                    // Scope automatically exits via RAII
                }
                context->setCurrentInstance(prevInstance);
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
        else
        {
            throw TypeException("Cannot access member '" + node->getMemberName() +
                "' on non-object value");
        }
    }

    Value ObjectEvaluator::accessMember(std::shared_ptr<ObjectInstance> object,
                                        const std::string& memberName)
    {
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

    void ObjectEvaluator::assignMember(std::shared_ptr<ObjectInstance> object,
                                       const std::string& memberName,
                                       const Value& value)
    {
        instanceManager->assignMember(object, memberName, value);
    }

    Value ObjectEvaluator::evaluateMethodCallNode(MethodCallNode* node)
    {
        if (!exprEvaluator)
        {
            throw TypeException("Expression evaluator not available for method call");
        }

        // Evaluate the object expression - delegate to expression evaluator for variables, but handle NewNode ourselves
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

        // Evaluate arguments
        std::vector<Value> args = evaluateArgumentList(node->getArguments());

        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue))
        {
            auto instance = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
            return callMethod(instance, node->getMethodName(), args);
        }
        else if (std::holds_alternative<std::shared_ptr<runtimeTypes::collections::Array>>(objectValue))
        {
            auto array = std::get<std::shared_ptr<runtimeTypes::collections::Array>>(objectValue);
            return callArrayMethod(array, node->getMethodName(), args);
        }
        else if (std::holds_alternative<std::shared_ptr<runtimeTypes::collections::Map>>(objectValue))
        {
            auto map = std::get<std::shared_ptr<runtimeTypes::collections::Map>>(objectValue);
            return callMapMethod(map, node->getMethodName(), args);
        }
        else if (std::holds_alternative<std::shared_ptr<runtimeTypes::collections::List>>(objectValue))
        {
            auto list = std::get<std::shared_ptr<runtimeTypes::collections::List>>(objectValue);
            return callCollectionMethod(list, node->getMethodName(), args);
        }
        else if (std::holds_alternative<std::shared_ptr<runtimeTypes::collections::Set>>(objectValue))
        {
            auto set = std::get<std::shared_ptr<runtimeTypes::collections::Set>>(objectValue);
            return callCollectionMethod(set, node->getMethodName(), args);
        }
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

        // Set current instance context
        auto prevInstance = context->getCurrentInstance();
        context->setCurrentInstance(object);

        // Use ScopeGuard and ParameterBinder utilities
        {
            utils::ScopeGuard scope(env, methodName, environment::manager::ScopeType::FUNCTION);

            try
            {
                // Use ParameterBinder utility instead of manual parameter binding
                utils::ParameterBinder::bindAndValidateParameters(
                    method->getParameters(),
                    args,
                    "method '" + methodName + "'",
                    env
                );

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
                context->setCurrentInstance(prevInstance);
                context->setReturned(false); // Reset return state after handling exception
                return e.returnValue;
            }
            catch (...)
            {
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

        // Find the class and method
        auto classDef = env->findClass(className);
        if (!classDef)
        {
            throw UndefinedException("Class '" + className + "' not found");
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

                return result;
            }
            catch (const exception::ReturnException& e)
            {
                // Handle return exception - extract return value
                context->setReturned(false); // Reset return state after handling exception
                return e.returnValue;
            }
            catch (...)
            {
                throw;
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

    // Collection method implementations
    template<typename CollectionType>
    Value ObjectEvaluator::callCollectionMethod(std::shared_ptr<CollectionType> collection,
                                                 const std::string& methodName,
                                                 const std::vector<Value>& args)
    {
        // Common collection methods
        if (methodName == "size")
        {
            if (!args.empty())
                throw TypeException("size() method takes no arguments");
            return static_cast<int>(collection->size());
        }
        else if (methodName == "empty")
        {
            if (!args.empty())
                throw TypeException("empty() method takes no arguments");
            return collection->empty();
        }
        else if (methodName == "clear")
        {
            if (!args.empty())
                throw TypeException("clear() method takes no arguments");
            collection->clear();
            return std::monostate{};
        }
        // Array-specific methods
        else if (methodName == "get")
        {
            if constexpr (std::is_same_v<CollectionType, runtimeTypes::collections::Array>) {
                if (args.size() != 1)
                    throw TypeException("get() method takes exactly 1 argument");
                if (!std::holds_alternative<int>(args[0]))
                    throw TypeException("Array index must be an integer");
                int index = std::get<int>(args[0]);
                if (index < 0)
                    throw TypeException("Array index cannot be negative");
                return collection->get(static_cast<size_t>(index));
            }
            else if constexpr (std::is_same_v<CollectionType, runtimeTypes::collections::Map>) {
                if (args.size() != 1)
                    throw TypeException("get() method takes exactly 1 argument");
                std::string key = exprEvaluator->toString(args[0]);
                return collection->get(key);
            }
            else {
                throw TypeException("get() method is not available on this collection type");
            }
        }
        else if (methodName == "set")
        {
            if constexpr (std::is_same_v<CollectionType, runtimeTypes::collections::Array>) {
                if (args.size() != 2)
                    throw TypeException("set() method takes exactly 2 arguments");
                if (!std::holds_alternative<int>(args[0]))
                    throw TypeException("Array index must be an integer");
                int index = std::get<int>(args[0]);
                if (index < 0)
                    throw TypeException("Array index cannot be negative");
                collection->set(static_cast<size_t>(index), args[1]);
                return std::monostate{};
            }
            else {
                throw TypeException("set() method is only available on Arrays");
            }
        }
        else if (methodName == "add" || methodName == "push")
        {
            if constexpr (std::is_same_v<CollectionType, runtimeTypes::collections::Array>) {
                if (args.size() != 1)
                    throw TypeException("add() method takes exactly 1 argument");
                collection->add(args[0]);
                return std::monostate{};
            }
            else {
                throw TypeException("add() method is only available on Arrays");
            }
        }
        // Map-specific methods
        else if (methodName == "put")
        {
            if constexpr (std::is_same_v<CollectionType, runtimeTypes::collections::Map>) {
                if (args.size() != 2)
                    throw TypeException("put() method takes exactly 2 arguments");
                std::string key = exprEvaluator->toString(args[0]);
                collection->put(key, args[1]);
                return std::monostate{};
            }
            else {
                throw TypeException("put() method is only available on Maps");
            }
        }
        else if (methodName == "containsKey")
        {
            if constexpr (std::is_same_v<CollectionType, runtimeTypes::collections::Map>) {
                if (args.size() != 1)
                    throw TypeException("containsKey() method takes exactly 1 argument");
                std::string key = exprEvaluator->toString(args[0]);
                return collection->containsKey(key);
            }
            else {
                throw TypeException("containsKey() method is only available on Maps");
            }
        }
        else if (methodName == "keySet")
        {
            if constexpr (std::is_same_v<CollectionType, runtimeTypes::collections::Map>) {
                if (!args.empty())
                    throw TypeException("keySet() method takes no arguments");
                return collection->keySet();
            }
            else {
                throw TypeException("keySet() method is only available on Maps");
            }
        }
        // Remove methods
        else if (methodName == "removeAt")
        {
            if constexpr (std::is_same_v<CollectionType, runtimeTypes::collections::Array>) {
                if (args.size() != 1)
                    throw TypeException("removeAt() method takes exactly 1 argument");
                if (!std::holds_alternative<int>(args[0]))
                    throw TypeException("Array index must be an integer");
                int index = std::get<int>(args[0]);
                if (index < 0)
                    throw TypeException("Array index cannot be negative");
                collection->removeAt(static_cast<size_t>(index));
                return std::monostate{};
            }
            else {
                throw TypeException("removeAt() method is only available on Arrays");
            }
        }
        else if (methodName == "remove")
        {
            if constexpr (std::is_same_v<CollectionType, runtimeTypes::collections::Map>) {
                if (args.size() != 1)
                    throw TypeException("remove() method takes exactly 1 argument");
                std::string key = exprEvaluator->toString(args[0]);
                collection->remove(key);
                return std::monostate{};
            }
            else {
                throw TypeException("remove() method is only available on Maps");
            }
        }
        else
        {
            throw TypeException("Unknown method '" + methodName + "' for collection type");
        }
    }

    // Specialized Array method operations
    Value ObjectEvaluator::callArrayMethod(std::shared_ptr<runtimeTypes::collections::Array> array,
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

    // Explicit template instantiations
    template Value ObjectEvaluator::callCollectionMethod<runtimeTypes::collections::Array>(
        std::shared_ptr<runtimeTypes::collections::Array>, const std::string&, const std::vector<Value>&);
    template Value ObjectEvaluator::callCollectionMethod<runtimeTypes::collections::Map>(
        std::shared_ptr<runtimeTypes::collections::Map>, const std::string&, const std::vector<Value>&);
    template Value ObjectEvaluator::callCollectionMethod<runtimeTypes::collections::List>(
        std::shared_ptr<runtimeTypes::collections::List>, const std::string&, const std::vector<Value>&);
    template Value ObjectEvaluator::callCollectionMethod<runtimeTypes::collections::Set>(
        std::shared_ptr<runtimeTypes::collections::Set>, const std::string&, const std::vector<Value>&);
}
