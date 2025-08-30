#include "ObjectEvaluator.hpp"
#include "Evaluator.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/UndefinedException.hpp"
#include "../errors/ArgumentException.hpp"
#include "../exception/ReturnException.hpp"
#include "../ast/nodes/classes/FieldNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../ast/nodes/classes/ConstructorNode.hpp"
#include "../runtimeTypes/klass/MethodDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../runtimeTypes/klass/ConstructorDefinition.hpp"

namespace evaluator
{
    using namespace errors;
    using namespace runtimeTypes::klass;

    ObjectEvaluator::ObjectEvaluator(Evaluator* evaluator)
        : mainEvaluator(evaluator), currentInstance(nullptr)
    {
    }

    Value ObjectEvaluator::evaluateClassNode(ClassNode* node)
    {
        auto env = mainEvaluator->getEnvironment();
        
        // Create class definition
        auto classDef = std::make_shared<ClassDefinition>(node->getClassName());
        
        // Add fields
        for (const auto& fieldPtr : node->getFields()) {
            auto fieldNode = dynamic_cast<FieldNode*>(fieldPtr.get());
            if (!fieldNode) continue;
            
            // Evaluate initial value if present
            Value initialValue{};
            if (fieldNode->hasInitialValue()) {
                initialValue = mainEvaluator->evaluate(fieldNode->getInitialValue());
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
        for (const auto& methodPtr : node->getMethods()) {
            auto methodNode = dynamic_cast<MethodNode*>(methodPtr.get());
            if (!methodNode) continue;
            
            auto methodDef = std::make_shared<MethodDefinition>(
                methodNode->getName(),
                methodNode->getReturnType(),
                methodNode->getParameters(),
                std::vector<std::pair<std::string, Value>>{}, // empty arguments
                methodNode->getBody(),
                methodNode->getIsStatic(),
                false // not final
            );
            
            classDef->addMethod(methodDef);
        }
        
        // Add constructors
        for (const auto& constructorPtr : node->getConstructors()) {
            auto constructorNode = dynamic_cast<ConstructorNode*>(constructorPtr.get());
            if (!constructorNode) continue;
            
            auto ctorDef = std::make_shared<ConstructorDefinition>(
                constructorNode->getParameters(),
                constructorNode->getBody()
            );
            
            classDef->addConstructor(ctorDef);
        }
        
        // Set namespace context on the class definition
        auto namespacePath = env->getCurrentNamespacePath();
        classDef->setNamespaceContext(namespacePath);
        
        // Register class
        env->registerClass(node->getClassName(), classDef);
        
        return std::monostate{};
    }

    Value ObjectEvaluator::evaluateMethodNode(MethodNode* node)
    {
        // Methods are evaluated as part of class evaluation
        return std::monostate{};
    }

    Value ObjectEvaluator::evaluateFieldNode(FieldNode* node)
    {
        // Fields are evaluated as part of class evaluation
        return std::monostate{};
    }

    Value ObjectEvaluator::evaluateConstructorNode(ConstructorNode* node)
    {
        // Constructors are evaluated as part of class evaluation
        return std::monostate{};
    }

    Value ObjectEvaluator::evaluateNewNode(NewNode* node)
    {
        auto env = mainEvaluator->getEnvironment();
        auto classDef = env->findClass(node->getClassName());
        
        if (!classDef) {
            throw UndefinedException("Undefined class: " + node->getClassName(), node->getLocation());
        }
        
        // Create new instance
        auto instance = std::make_shared<ObjectInstance>(classDef);
        
        // Evaluate constructor arguments
        std::vector<Value> args;
        for (const auto& argNode : node->getArguments()) {
            args.push_back(mainEvaluator->evaluate(argNode.get()));
        }
        
        // Find matching constructor
        auto constructor = classDef->findConstructor(args.size());
        
        if (!constructor) {
            // If no constructor found, check if this is a default constructor call on a class with no constructors
            if (args.size() == 0 && classDef->getConstructorCount() == 0) {
                // Allow default constructor for classes with no explicit constructors
                // No constructor execution needed - just return the instance
                return instance;
            }
            
            throw UndefinedException("No matching constructor found for class " + node->getClassName() + 
                " with " + std::to_string(args.size()) + " arguments", node->getLocation());
        }
        
        // Save current namespace context and enter the class's namespace
        auto savedNamespacePath = env->getCurrentNamespacePath();
        auto classNamespace = classDef->getNamespaceContext();
        if (!classNamespace.empty()) {
            env->enterNamespace(classNamespace);
        }
        
        // Create scope for constructor execution
        env->enterScope(node->getClassName() + "::<constructor>", ScopeType::FUNCTION);
        
        // Save the current instance (if any) and set new instance for 'this' access
        auto savedCurrentInstance = getCurrentInstance();
        setCurrentInstance(instance);
        
        // Mark that we're in constructor context for final field assignment
        auto constructorContextVar = std::make_shared<VariableDefinition>(
            "__in_constructor__", ValueType::BOOL, Value(true), true);
        env->declareVariable("__in_constructor__", constructorContextVar);
        
        // Bind constructor parameters
        for (size_t i = 0; i < args.size(); ++i) {
            const auto& param = constructor->getParameters()[i];
            auto varDef = std::make_shared<VariableDefinition>(
                param.first,   // parameter name
                param.second,  // parameter type
                args[i],       // argument value
                false          // not final
            );
            env->declareVariable(param.first, varDef);
        }
        
        // Execute constructor body
        try {
            mainEvaluator->evaluate(constructor->getBody());
        }
        catch (...) {
            // Restore the saved current instance
            setCurrentInstance(savedCurrentInstance);
            env->exitScope();
            
            // Restore namespace context
            if (!classNamespace.empty()) {
                env->exitNamespace();
                if (!savedNamespacePath.empty()) {
                    env->enterNamespace(savedNamespacePath);
                }
            }
            
            throw;
        }
        
        // Restore the saved current instance
        setCurrentInstance(savedCurrentInstance);
        env->exitScope();
        
        // Restore namespace context
        if (!classNamespace.empty()) {
            env->exitNamespace();
            if (!savedNamespacePath.empty()) {
                env->enterNamespace(savedNamespacePath);
            } else {
                // Ensure we're in global namespace - fix for scope stack imbalance
                while (!env->getCurrentNamespacePath().empty()) {
                    env->exitNamespace();
                }
            }
        }
        
        return instance;
    }

    Value ObjectEvaluator::evaluateMemberAccessNode(MemberAccessNode* node)
    {
        Value objectValue = mainEvaluator->evaluate(node->getObject());
        
        if (std::holds_alternative<nullptr_t>(objectValue)) {
            throw TypeException("Cannot access member of null object", node->getLocation());
        }
        
        if (!std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue)) {
            throw TypeException("Cannot access member of non-object value", node->getLocation());
        }
        
        auto object = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
        return accessMember(object, node->getMemberName());
    }

    Value ObjectEvaluator::evaluateMethodCallNode(MethodCallNode* node)
    {
        Value objectValue = mainEvaluator->evaluate(node->getObject());
        
        if (std::holds_alternative<nullptr_t>(objectValue)) {
            throw TypeException("Cannot call method on null object", node->getLocation());
        }
        
        if (!std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue)) {
            throw TypeException("Cannot call method on non-object value", node->getLocation());
        }
        
        auto object = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
        
        // Evaluate arguments
        std::vector<Value> args;
        for (const auto& argNode : node->getArguments()) {
            args.push_back(mainEvaluator->evaluate(argNode.get()));
        }
        
        return callMethod(object, node->getMethodName(), args);
    }

    Value ObjectEvaluator::evaluateMemberAssignmentNode(MemberAssignmentNode* node)
    {
        Value objectValue = mainEvaluator->evaluate(node->getObject());
        
        if (std::holds_alternative<nullptr_t>(objectValue)) {
            throw TypeException("Cannot assign member of null object", node->getLocation());
        }
        
        if (!std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue)) {
            throw TypeException("Cannot assign member of non-object value", node->getLocation());
        }
        
        auto object = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
        Value newValue = mainEvaluator->evaluate(node->getValue());
        
        assignMember(object, node->getMemberName(), newValue);
        return newValue;
    }

    // Helper methods
    std::shared_ptr<ObjectInstance> ObjectEvaluator::createInstance(const std::string& className,
                                                                    const std::vector<Value>& constructorArgs)
    {
        auto env = mainEvaluator->getEnvironment();
        auto classDef = env->findClass(className);
        
        if (!classDef) {
            throw UndefinedException("Undefined class: " + className, SourceLocation{});
        }
        
        auto instance = std::make_shared<ObjectInstance>(classDef);
        
        // TODO: Execute constructor with args
        
        return instance;
    }

    Value ObjectEvaluator::accessMember(std::shared_ptr<ObjectInstance> object, const std::string& memberName)
    {
        auto field = object->getField(memberName);
        
        if (!field) {
            throw UndefinedException("Undefined field: " + memberName, SourceLocation{});
        }
        
        // Return the instance's field value, not the class definition's default value
        return object->getFieldValue(memberName);
    }

    void ObjectEvaluator::assignMember(std::shared_ptr<ObjectInstance> object, const std::string& memberName,
                                       const Value& value)
    {
        auto field = object->getField(memberName);
        
        if (!field) {
            throw UndefinedException("Undefined field: " + memberName, SourceLocation{});
        }
        
        if (field->isFinal()) {
            throw TypeException("Cannot reassign final field: " + memberName, SourceLocation{});
        }
        
        // Set the value on the object instance, not the field definition
        object->setField(memberName, value);
    }

    Value ObjectEvaluator::callMethod(std::shared_ptr<ObjectInstance> object, const std::string& methodName,
                                      const std::vector<Value>& args)
    {
        auto method = object->getClassDefinition()->getMethod(methodName);
        
        if (!method) {
            throw UndefinedException("Undefined method: " + methodName, SourceLocation{});
        }
        
        // Check if trying to call static method through instance
        if (method->isStatic()) {
            throw TypeException("Cannot call static method '" + methodName + 
                              "' through instance. Use class name instead.", SourceLocation{});
        }
        
        // Check parameter count
        if (args.size() != method->getParameters().size()) {
            throw ArgumentException("Method '" + methodName + 
                                  "' expects " + std::to_string(method->getParameters().size()) +
                                  " arguments, got " + std::to_string(args.size()),
                                  SourceLocation{});
        }
        
        auto env = mainEvaluator->getEnvironment();
        
        // Save the current return state to isolate method execution
        bool savedReturnState = mainEvaluator->shouldReturn();
        
        // Save current namespace context and enter the class's namespace
        auto savedNamespacePath = env->getCurrentNamespacePath();
        auto classNamespace = object->getClassDefinition()->getNamespaceContext();
        if (!classNamespace.empty()) {
            env->enterNamespace(classNamespace);
        }
        
        env->enterScope(object->getClassDefinition()->getName() + "::" + methodName, ScopeType::FUNCTION);
        
        // Save previous instance and set current instance
        auto savedInstance = getCurrentInstance();
        setCurrentInstance(object);
        
        // Bind parameters
        for (size_t i = 0; i < args.size(); ++i) {
            const auto& param = method->getParameters()[i];
            auto varDef = std::make_shared<VariableDefinition>(
                param.first,   // parameter name
                param.second,  // parameter type
                args[i],       // argument value
                false          // not final
            );
            env->declareVariable(param.first, varDef);
        }
        
        // Reset return state for method execution
        mainEvaluator->setReturned(false);
        
        // Execute method body
        Value result = std::monostate{};
        try {
            mainEvaluator->evaluate(method->getBody());
            
            if (mainEvaluator->shouldReturn()) {
                result = mainEvaluator->getReturnValue();
            }
        }
        catch (const exception::ReturnException& e) {
            // This is expected - return statement was executed
            result = e.returnValue;
        }
        catch (...) {
            setCurrentInstance(savedInstance);
            env->exitScope();
            
            // Restore namespace context
            if (!classNamespace.empty()) {
                env->exitNamespace();
                if (!savedNamespacePath.empty()) {
                    env->enterNamespace(savedNamespacePath);
                }
            }
            
            // Restore return state before throwing
            mainEvaluator->setReturned(savedReturnState);
            throw;
        }
        
        setCurrentInstance(savedInstance);
        env->exitScope();
        
        // Restore namespace context
        if (!classNamespace.empty()) {
            env->exitNamespace();
            if (!savedNamespacePath.empty()) {
                env->enterNamespace(savedNamespacePath);
            } else {
                // Ensure we're in global namespace - fix for scope stack imbalance
                while (!env->getCurrentNamespacePath().empty()) {
                    env->exitNamespace();
                }
            }
        }
        
        // Restore the original return state to not affect outer context
        mainEvaluator->setReturned(savedReturnState);
        
        return result;
    }

    void ObjectEvaluator::setCurrentInstance(std::shared_ptr<ObjectInstance> instance)
    {
        currentInstance = instance;
    }

    std::shared_ptr<ObjectInstance> ObjectEvaluator::getCurrentInstance() const
    {
        return currentInstance;
    }

    void ObjectEvaluator::clearCurrentInstance()
    {
        currentInstance = nullptr;
    }

    Value ObjectEvaluator::accessStaticMember(const std::string& className, const std::string& memberName)
    {
        // TODO: Implement static member access
        return std::monostate{};
    }

    void ObjectEvaluator::assignStaticMember(const std::string& className, const std::string& memberName,
                                            const Value& value)
    {
        // TODO: Implement static member assignment
    }

    Value ObjectEvaluator::callStaticMethod(const std::string& className, const std::string& methodName,
                                           const std::vector<Value>& args)
    {
        // TODO: Implement static method calls
        return std::monostate{};
    }
}