#include "ObjectEvaluator.hpp"
#include "base/IEvaluator.hpp"
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
#include "../runtimeTypes/klass/ConstructorDefinition.hpp"

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
        if (!node || !canHandle(node)) {
            return std::monostate{};
        }
        
        // Dispatch to appropriate evaluation method based on node type
        if (auto classNode = dynamic_cast<ClassNode*>(node)) {
            return evaluateClassNode(classNode);
        }
        if (auto methodNode = dynamic_cast<MethodNode*>(node)) {
            return evaluateMethodNode(methodNode);
        }
        if (auto fieldNode = dynamic_cast<FieldNode*>(node)) {
            return evaluateFieldNode(fieldNode);
        }
        if (auto ctorNode = dynamic_cast<ConstructorNode*>(node)) {
            return evaluateConstructorNode(ctorNode);
        }
        if (auto newNode = dynamic_cast<NewNode*>(node)) {
            return evaluateNewNode(newNode);
        }
        if (auto memberAccessNode = dynamic_cast<MemberAccessNode*>(node)) {
            return evaluateMemberAccessNode(memberAccessNode);
        }
        if (auto methodCallNode = dynamic_cast<MethodCallNode*>(node)) {
            return evaluateMethodCallNode(methodCallNode);
        }
        if (auto memberAssignNode = dynamic_cast<MemberAssignmentNode*>(node)) {
            return evaluateMemberAssignmentNode(memberAssignNode);
        }
        
        return std::monostate{};
    }
    
    bool ObjectEvaluator::canHandle(ASTNode* node) const
    {
        return isObjectNode(node);
    }
    
    
    void ObjectEvaluator::setExpressionEvaluator(IExpressionEvaluator* evaluator)
    {
        exprEvaluator = evaluator;
    }
    
    void ObjectEvaluator::setStatementEvaluator(IStatementEvaluator* evaluator)
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
        for (const auto& fieldPtr : node->getFields()) {
            auto fieldNode = dynamic_cast<FieldNode*>(fieldPtr.get());
            if (!fieldNode) continue;
            
            // Evaluate initial value if present
            Value initialValue{};
            if (fieldNode->hasInitialValue() && exprEvaluator) {
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
        for (const auto& methodPtr : node->getMethods()) {
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
        for (const auto& constructorPtr : node->getConstructors()) {
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
        if (exprEvaluator) {
            args = evaluateArgumentList(node->getArguments());
        }
        
        auto instance = createInstance(node->getClassName(), args);
        
        // Execute constructor if it exists
        auto env = context->getEnvironment();
        auto classDef = env->findClass(node->getClassName());
        if (classDef && !classDef->getConstructors().empty()) {
            auto constructor = classDef->findConstructor(args.size());
            if (constructor && constructor->getBody()) {
                // Set the current instance for constructor execution
                auto prevInstance = context->getCurrentInstance();
                context->setCurrentInstance(instance);
                
                // Use ScopeGuard for automatic scope management
                {
                    utils::ScopeGuard scope(env, "constructor", environment::manager::ScopeType::FUNCTION);
                    
                    try {
                        // Use ParameterBinder utility to eliminate duplication
                        utils::ParameterBinder::bindAndValidateParameters(
                            constructor->getParameters(),
                            args,
                            "constructor for class '" + node->getClassName() + "'",
                            env
                        );
                        
                        // Execute constructor body
                        if (stmtEvaluator) {
                            auto bodyPtr = constructor->getBody();
                            if (bodyPtr) {
                                stmtEvaluator->evaluate(bodyPtr);
                            }
                        }
                    }
                    catch (...) {
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
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(result)) {
            // Good - we have an object
        } else if (std::holds_alternative<std::monostate>(result)) {
            // Bad - we have void
            throw TypeException("Object creation returned void instead of instance");
        } else {
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
        if (!exprEvaluator) {
            throw TypeException("Expression evaluator not available for member access");
        }
        
        // Evaluate the object expression
        Value objectValue = exprEvaluator->evaluate(node->getObject());
        
        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue)) {
            auto instance = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
            return accessMember(instance, node->getMemberName());
        } else {
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
        if (!exprEvaluator) {
            throw TypeException("Expression evaluator not available for member assignment");
        }
        
        // Evaluate the object expression
        Value objectValue = exprEvaluator->evaluate(node->getObject());
        
        // Evaluate the new value
        Value newValue = exprEvaluator->evaluate(node->getValue());
        
        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue)) {
            auto instance = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
            assignMember(instance, node->getMemberName(), newValue);
            return newValue;
        } else {
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
        if (!exprEvaluator) {
            throw TypeException("Expression evaluator not available for method call");
        }
        
        // Evaluate the object expression - delegate to expression evaluator for variables, but handle NewNode ourselves
        Value objectValue;
        if (dynamic_cast<NewNode*>(node->getObject())) {
            // Handle NewNode directly since ExpressionEvaluator would delegate back to us anyway
            objectValue = evaluate(node->getObject());
        } else {
            // Delegate to expression evaluator for other nodes (like VariableNode)
            objectValue = exprEvaluator->evaluate(node->getObject());
        }
        
        // Evaluate arguments
        std::vector<Value> args = evaluateArgumentList(node->getArguments());
        
        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(objectValue)) {
            auto instance = std::get<std::shared_ptr<ObjectInstance>>(objectValue);
            return callMethod(instance, node->getMethodName(), args);
        } else {
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
        if (!classDef) {
            throw UndefinedException("Object has no class definition");
        }
        
        // Find the method
        auto method = classDef->findMethod(methodName, args.size());
        if (!method) {
            throw UndefinedException("Method '" + methodName + "' not found in class '" + 
                                   classDef->getName() + "'");
        }
        
        // Check if trying to call static method on instance
        if (method->isStatic()) {
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
        
            try {
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
            if (method->getBody() && stmtEvaluator) {
                stmtEvaluator->evaluate(method->getBody());
            }
            
            // Get return value if method returned
            if (context->shouldReturn()) {
                result = context->getReturnValue();
                context->setReturned(false);
            }
            
                context->setCurrentInstance(prevInstance);
                return result;
                
            } catch (const exception::ReturnException& e) {
                // Handle return exception - extract return value
                context->setCurrentInstance(prevInstance);
                context->setReturned(false); // Reset return state after handling exception
                return e.returnValue;
            } catch (...) {
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
        
        if (exprEvaluator) {
            for (const auto& arg : args) {
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
        if (!classDef) {
            throw UndefinedException("Class '" + className + "' not found");
        }
        
        // Try to find method with exact argument count first
        auto method = classDef->findMethod(methodName, args.size());
        
        // If not found, try without argument count matching as a fallback
        if (!method) {
            method = classDef->getMethod(methodName);
        }
        
        // Check if method exists and is static
        if (!method) {
            throw UndefinedException("Method '" + methodName + "' not found in class '" + className + "'");
        }
        
        if (!method->isStatic()) {
            throw UndefinedException("Method '" + methodName + "' in class '" + className + "' is not static");
        }
        
        // Use ScopeGuard and ParameterBinder utilities
        {
            utils::ScopeGuard scope(env, methodName, environment::manager::ScopeType::FUNCTION);
            
            try {
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
                if (method->getBody() && stmtEvaluator) {
                    stmtEvaluator->evaluate(method->getBody());
                }
                
                // Get return value if method returned
                if (context->shouldReturn()) {
                    result = context->getReturnValue();
                    context->setReturned(false);
                }
                
                return result;
                
            } catch (const exception::ReturnException& e) {
                // Handle return exception - extract return value
                context->setReturned(false); // Reset return state after handling exception
                return e.returnValue;
            } catch (...) {
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
}