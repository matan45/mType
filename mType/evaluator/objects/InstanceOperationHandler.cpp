#include "InstanceOperationHandler.hpp"
#include "../StatementEvaluator.hpp"
#include "../utils/ScopeGuard.hpp"
#include "../utils/ParameterBinder.hpp"
#include "../../constants/LambdaConstants.hpp"
#include "../../value/LambdaValue.hpp"
#include "../../value/ParameterType.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../errors/ReturnException.hpp"

using namespace errors;

namespace evaluator {
namespace objects {

    std::shared_ptr<ObjectInstance> InstanceOperationHandler::createInstance(
        const std::string& className,
        const std::vector<Value>& constructorArgs)
    {
        auto env = context->getEnvironment();
        return instanceManager->createInstance(className, constructorArgs, env);
    }

    std::shared_ptr<ObjectInstance> InstanceOperationHandler::createInstanceWithTypeBindings(
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

    Value InstanceOperationHandler::accessMember(std::shared_ptr<ObjectInstance> object,
                                                 const std::string& memberName,
                                                 const SourceLocation& location)
    {
        if (!object)
        {
            throw TypeException("Cannot access member '" + memberName + "' on null object");
        }

        // Get the field definition
        auto field = object->getField(memberName);
        if (!field)
        {
            throw UndefinedException("Field '" + memberName + "' not found in class '" +
                                     object->getClassDefinition()->getName() + "'");
        }

        // VALIDATION: Prevent instance member access from static methods
        if (context->isInStaticMethodContext())
        {
            if (!field->isStatic())
            {
                throw TypeException("Cannot access instance field '" + memberName +
                                    "' from static method context",
                                    location);
            }
        }

        // ACCESS CONTROL: Validate field access permissions
        auto classDef = object->getClassDefinition();
        auto callingInstance = context->getCurrentInstance();
        auto accessContext = base::AccessContext::forInstanceAccess(
            callingInstance,
            classDef,
            location
        );
        validation::AccessValidator::validateFieldAccess(accessContext, *field);

        auto env = context->getEnvironment();
        return instanceManager->accessMember(object, memberName, location);
    }

    void InstanceOperationHandler::assignMember(std::shared_ptr<ObjectInstance> object,
                                                const std::string& memberName,
                                                const Value& value,
                                                const SourceLocation& location)
    {
        if (!object)
        {
            throw TypeException("Cannot assign to member '" + memberName + "' on null object");
        }

        // Get the field definition
        auto field = object->getField(memberName);
        if (!field)
        {
            throw UndefinedException("Field '" + memberName + "' not found in class '" +
                                     object->getClassDefinition()->getName() + "'");
        }

        // VALIDATION: Prevent instance member assignment from static methods
        if (context->isInStaticMethodContext())
        {
            if (!field->isStatic())
            {
                throw TypeException("Cannot assign to instance field '" + memberName +
                                    "' from static method context",
                                    location);
            }
        }

        // ACCESS CONTROL: Validate field access permissions
        auto classDef = object->getClassDefinition();
        auto callingInstance = context->getCurrentInstance();
        auto accessContext = base::AccessContext::forInstanceAccess(
            callingInstance,
            classDef,
            location
        );
        validation::AccessValidator::validateFieldAccess(accessContext, *field);

        instanceManager->assignMember(object, memberName, value, location);
    }

    Value InstanceOperationHandler::callMethod(std::shared_ptr<ObjectInstance> object,
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

        // Find the method (search in class hierarchy for inherited methods)
        auto method = classDef->findMethodInHierarchy(methodName, args.size());
        if (!method)
        {
            throw UndefinedException("Method '" + methodName + "' not found in class '" +
                classDef->getName() + "'");
        }

        // ACCESS CONTROL: Validate method access permissions
        auto callingInstance = context->getCurrentInstance();
        auto accessContext = base::AccessContext::forInstanceAccess(
            callingInstance,
            classDef,
            location
        );
        validation::AccessValidator::validateMethodAccess(accessContext, *method);

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
            // Enhanced debugging and validation
            if (method->needsLambdaCleanup()) {
                std::string status = method->getLambdaLifecycleStatus();
                throw TypeException("Method '" + methodName + "' has invalid lambda state and needs cleanup. " +
                                  "Status: " + status);
            }

            auto lambdaNode = method->getLambdaNode();
            if (lambdaNode && method->isLambdaNodeValid())
            {
                // Create LambdaValue with current evaluation context
                // Convert shared_ptr to raw pointer for LambdaValue constructor
                auto rawLambdaNode = lambdaNode.get();
                if (!rawLambdaNode) {
                    throw TypeException("Lambda node is null - cannot create lambda for method '" + methodName + "'");
                }
                auto lambdaValueObj = std::make_shared<value::LambdaValue>(rawLambdaNode, context);

                // Set interface implementation info
                lambdaValueObj->setInterfaceImplementation(classDef->getName(), methodName);

                // Invoke the lambda with proper parameter mapping
                try {
                    Value result = lambdaValueObj->invoke(convertedArgs, context);
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
                                    location);
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

        // Push calling class onto stack for access control
        context->pushCallingClass(classDef->getName());

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

                // Pop calling class from stack
                context->popCallingClass();

                context->setCurrentInstance(prevInstance);

                // Restore previous generic type bindings
                context->setGenericTypeBindings(prevGenericBindings);

                return result;
            }
            catch (const ReturnException& e)
            {
                // Handle return exception - extract return value
                context->setInStaticMethod(wasInStaticMethod); // Restore static method context
                context->popCallingClass(); // Pop calling class from stack
                context->setCurrentInstance(prevInstance);
                context->setGenericTypeBindings(prevGenericBindings); // Restore generic bindings
                context->setReturned(false); // Reset return state after handling exception
                return e.returnValue;
            }
            catch (...)
            {
                context->setInStaticMethod(wasInStaticMethod); // Restore static method context
                context->popCallingClass(); // Pop calling class from stack
                context->setCurrentInstance(prevInstance);
                context->setGenericTypeBindings(prevGenericBindings); // Restore generic bindings
                throw;
            }
            // Scope automatically exits via RAII
        }
    }

} // namespace objects
} // namespace evaluator
