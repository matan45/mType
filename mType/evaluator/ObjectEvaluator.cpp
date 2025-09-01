#include "ObjectEvaluator.hpp"
#include "Evaluator.hpp"
#include "StatementEvaluator.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/UndefinedException.hpp"
#include "../errors/ArgumentException.hpp"
#include "../exception/ReturnException.hpp"
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
            
            // Get reference to AST body from MethodNode (ownership remains with MethodNode)
            auto bodyPtr = std::shared_ptr<ASTNode>(methodNode->getBody(), [](ASTNode*){});
            
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
        for (const auto& constructorPtr : node->getConstructors()) {
            auto constructorNode = dynamic_cast<ConstructorNode*>(constructorPtr.get());
            if (!constructorNode) continue;
            
            // Get reference to AST body from ConstructorNode (ownership remains with ConstructorNode)
            auto bodyPtr = std::shared_ptr<ASTNode>(constructorNode->getBody(), [](ASTNode*){});
            
            auto ctorDef = std::make_shared<ConstructorDefinition>(
                constructorNode->getParameters(),
                bodyPtr
            );
            
            classDef->addConstructor(ctorDef);
        }
        
        // Namespace context disabled - register class globally
        // auto namespacePath = env->getCurrentNamespacePath();
        // classDef->setNamespaceContext(namespacePath);
        
        // Register class in global scope (namespace support removed)
        auto classRegistry = env->getClassRegistry();
        if (classRegistry) {
            classRegistry->registerItem(node->getClassName(), classDef);
        }
        
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
        
        // Create scope for constructor execution
        env->enterScope(node->getClassName() + "::<constructor>", ScopeType::FUNCTION);
        
        // Save the current instance (if any) and set new instance for 'this' access
        auto savedCurrentInstance = getCurrentInstance();
        setCurrentInstance(instance);
        
        // Mark that we're in constructor context for final field assignment
        auto constructorContextVar = std::make_shared<VariableDefinition>(
            "__in_constructor__", ValueType::BOOL, Value(true), true);
        env->declareVariable("__in_constructor__", constructorContextVar);
        
        // Store current class name for static field access during constructor execution
        auto classNameVar = std::make_shared<VariableDefinition>(
            "__current_class_name__", ValueType::STRING, Value(node->getClassName()), true);
        env->declareVariable("__current_class_name__", classNameVar);
        
        // Bind constructor parameters
        for (size_t i = 0; i < args.size(); ++i) {
            const auto& param = constructor->getParameters()[i];
            
            // Type checking: verify argument type matches parameter type
            ValueType actualType = mainEvaluator->getStatementEvaluator()->getValueType(args[i]);
            ValueType parameterType = param.second;
            
            // Type checking with specific rules
            if (actualType != parameterType) {
                // Allow int to float conversion
                if (actualType == ValueType::INT && parameterType == ValueType::FLOAT) {
                    // This is allowed
                }
                // Allow null assignment only to object types
                else if (actualType == ValueType::NULL_TYPE && parameterType == ValueType::OBJECT) {
                    // This is allowed
                }
                else {
                    throw errors::TypeException("Type mismatch in constructor for class '" + classDef->getName() + "': parameter '" + 
                                               param.first + "' expects " + mainEvaluator->getStatementEvaluator()->valueTypeToString(parameterType) + 
                                               " but got " + mainEvaluator->getStatementEvaluator()->valueTypeToString(actualType), node->getLocation());
                }
            }
            
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
            
            throw;
        }
        
        // Restore the saved current instance
        setCurrentInstance(savedCurrentInstance);
        env->exitScope();
        
        
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
        
        
        
        env->enterScope(object->getClassDefinition()->getName() + "::" + methodName, ScopeType::FUNCTION);
        
        // Save previous instance and set current instance
        auto savedInstance = getCurrentInstance();
        setCurrentInstance(object);
        
        // Bind parameters
        for (size_t i = 0; i < args.size(); ++i) {
            const auto& param = method->getParameters()[i];
            
            // Type checking: verify argument type matches parameter type
            ValueType actualType = mainEvaluator->getStatementEvaluator()->getValueType(args[i]);
            ValueType parameterType = param.second;
            
            // Type checking with specific rules
            if (actualType != parameterType) {
                // Allow int to float conversion
                if (actualType == ValueType::INT && parameterType == ValueType::FLOAT) {
                    // This is allowed
                }
                // Allow null assignment only to object types
                else if (actualType == ValueType::NULL_TYPE && parameterType == ValueType::OBJECT) {
                    // This is allowed
                }
                else {
                    throw errors::TypeException("Type mismatch in method '" + methodName + "' of class '" + object->getClassDefinition()->getName() + "': parameter '" + 
                                               param.first + "' expects " + mainEvaluator->getStatementEvaluator()->valueTypeToString(parameterType) + 
                                               " but got " + mainEvaluator->getStatementEvaluator()->valueTypeToString(actualType), SourceLocation{});
                }
            }
            
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
            
            
            
            // Restore return state before throwing
            mainEvaluator->setReturned(savedReturnState);
            throw;
        }
        
        setCurrentInstance(savedInstance);
        env->exitScope();
        
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
        auto env = mainEvaluator->getEnvironment();
        auto classDef = env->findClass(className);
        
        if (!classDef) {
            throw UndefinedException("Class '" + className + "' not found", SourceLocation{});
        }
        
        // Find the static field
        auto field = classDef->getField(memberName);
        if (!field) {
            throw UndefinedException("Field '" + memberName + "' not found in class '" + className + "'", SourceLocation{});
        }
        
        if (!field->isStatic()) {
            throw TypeException("Field '" + memberName + "' in class '" + className + "' is not static", SourceLocation{});
        }
        
        return field->getValue();
    }

    void ObjectEvaluator::assignStaticMember(const std::string& className, const std::string& memberName,
                                            const Value& value)
    {
        auto env = mainEvaluator->getEnvironment();
        auto classDef = env->findClass(className);
        
        if (!classDef) {
            throw UndefinedException("Class '" + className + "' not found", SourceLocation{});
        }
        
        // Find the static field
        auto field = classDef->getField(memberName);
        if (!field) {
            throw UndefinedException("Field '" + memberName + "' not found in class '" + className + "'", SourceLocation{});
        }
        
        if (!field->isStatic()) {
            throw TypeException("Field '" + memberName + "' in class '" + className + "' is not static", SourceLocation{});
        }
        
        if (field->isFinal()) {
            throw TypeException("Cannot reassign final static field: " + className + "::" + memberName, SourceLocation{});
        }
        
        // Update the field value (static fields are stored in the class definition)
        field->setValue(value);
    }

    Value ObjectEvaluator::callStaticMethod(const std::string& className, const std::string& methodName,
                                           const std::vector<Value>& args)
    {
        auto env = mainEvaluator->getEnvironment();
        auto classDef = env->findClass(className);
        
        if (!classDef) {
            throw UndefinedException("Class '" + className + "' not found", SourceLocation{});
        }
        
        // Find the static method
        auto method = classDef->getMethod(methodName);
        if (!method) {
            throw UndefinedException("Method '" + methodName + "' not found in class '" + className + "'", SourceLocation{});
        }
        
        if (!method->isStatic()) {
            throw TypeException("Method '" + methodName + "' in class '" + className + "' is not static", SourceLocation{});
        }
        
        // Check parameter count
        if (args.size() != method->getParameters().size()) {
            throw ArgumentException("Static method '" + className + "::" + methodName + 
                                  "' expects " + std::to_string(method->getParameters().size()) +
                                  " arguments, got " + std::to_string(args.size()),
                                  SourceLocation{});
        }
        
        // Create scope for static method execution
        env->enterScope(className + "::" + methodName, ScopeType::FUNCTION);
        
        // Save the current return state to isolate method execution
        bool savedReturnState = mainEvaluator->shouldReturn();
        
        // Store current class name for static field access
        auto classNameVar = std::make_shared<VariableDefinition>(
            "__current_class_name__", ValueType::STRING, Value(className), true);
        env->declareVariable("__current_class_name__", classNameVar);
        
        // Bind parameters
        for (size_t i = 0; i < args.size(); ++i) {
            const auto& param = method->getParameters()[i];
            
            // Type checking: verify argument type matches parameter type
            ValueType actualType = mainEvaluator->getStatementEvaluator()->getValueType(args[i]);
            ValueType parameterType = param.second;
            
            // Type checking with specific rules
            if (actualType != parameterType) {
                // Allow int to float conversion
                if (actualType == ValueType::INT && parameterType == ValueType::FLOAT) {
                    // This is allowed
                }
                // Allow null assignment only to object types
                else if (actualType == ValueType::NULL_TYPE && parameterType == ValueType::OBJECT) {
                    // This is allowed
                }
                else {
                    throw errors::TypeException("Type mismatch in static method '" + className + "::" + methodName + "': parameter '" + 
                                               param.first + "' expects " + mainEvaluator->getStatementEvaluator()->valueTypeToString(parameterType) + 
                                               " but got " + mainEvaluator->getStatementEvaluator()->valueTypeToString(actualType), SourceLocation{});
                }
            }
            
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
        
        // Execute static method body
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
            env->exitScope();
            
            // Restore return state before throwing
            mainEvaluator->setReturned(savedReturnState);
            throw;
        }
        
        env->exitScope();
        
        // Restore the original return state to not affect outer context
        mainEvaluator->setReturned(savedReturnState);
        
        return result;
    }
}