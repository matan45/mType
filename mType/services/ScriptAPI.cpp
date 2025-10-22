#include "ScriptAPI.hpp"
#include "../evaluator/Evaluator.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include "../errors/MethodNotFoundException.hpp"
#include "../errors/ObjectException.hpp"
#include "../errors/ClassNotFoundException.hpp"
#include "../errors/FieldNotFoundException.hpp"
#include "../errors/FinalModificationException.hpp"
#include "../errors/ParameterMismatchException.hpp"
#include "../errors/TypeConversionException.hpp"
#include "../errors/ReturnException.hpp"
#include <iostream>

namespace services
{
    ScriptAPI::ScriptAPI(std::shared_ptr<environment::Environment> env, evaluator::Evaluator* eval)
        : environment(env), evaluator(eval)
    {
    }

    ScriptAPI::~ScriptAPI() = default;

    value::Value ScriptAPI::callFunction(const std::string& functionName, const std::vector<value::Value>& args)
    {
        // First try to find a regular mType function
        auto funcDef = environment->findFunction(functionName);
        if (funcDef)
        {
            return invokeFunction(funcDef, args);
        }

        // If not found, try to find a native function
        auto nativeRegistry = environment->getNativeRegistry();
        if (nativeRegistry && nativeRegistry->hasNativeFunction(functionName))
        {
            auto nativeFunc = nativeRegistry->findNativeFunction(functionName);
            if (nativeFunc)
            {
                return nativeFunc(args);
            }
        }

        throw errors::MethodNotFoundException(functionName, "", __FUNCTION__);
    }

    value::Value ScriptAPI::callMethod(const value::Value& object,
                                       const std::string& methodName,
                                       const std::vector<value::Value>& args)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            throw errors::ObjectException("Cannot call method on non-object value", "", __FUNCTION__);
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        return evaluator->callMethodOnInstance(instance, methodName, args);
    }

    value::Value ScriptAPI::callStaticMethod(const std::string& className,
                                             const std::string& methodName,
                                             const std::vector<value::Value>& args)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        return invokeStaticMethod(classDef, methodName, args);
    }

    value::Value ScriptAPI::getStaticField(const std::string& className, const std::string& fieldName)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        auto field = classDef->getField(fieldName);
        if (!field || !field->isStatic())
        {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        return field->getValue();
    }

    void ScriptAPI::setStaticField(const std::string& className,
                                   const std::string& fieldName,
                                   const value::Value& value)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        auto field = classDef->getField(fieldName);
        if (!field || !field->isStatic())
        {
            throw errors::FieldNotFoundException(fieldName, className);
        }

        if (field->isFinal())
        {
            throw errors::FinalModificationException(fieldName, className);
        }

        field->setValue(value);
    }

    value::Value ScriptAPI::getVariable(const std::string& variableName)
    {
        auto varDef = environment->findVariable(variableName);
        if (!varDef)
        {
            throw errors::FieldNotFoundException(variableName);
        }

        return varDef->getValue();
    }

    void ScriptAPI::setVariable(const std::string& variableName, const value::Value& value)
    {
        auto varDef = environment->findVariable(variableName);
        if (!varDef)
        {
            throw errors::FieldNotFoundException(variableName);
        }

        if (varDef->isFinal())
        {
            throw errors::FinalModificationException(variableName);
        }

        varDef->setValue(value);
    }

    value::Value ScriptAPI::createObject(const std::string& className,
                                         const std::vector<value::Value>& constructorArgs)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw errors::ClassNotFoundException(className);
        }

        // Create the object instance
        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);

        // Find and call appropriate constructor
        auto constructor = classDef->findConstructor(constructorArgs.size());
        if (constructor)
        {
            // Set up parameters in a new scope
            environment->enterScope("constructor");

            auto params = constructor->getParameters();
            if (params.size() != constructorArgs.size())
            {
                environment->exitScope();
                throw errors::ParameterMismatchException("constructor",
                                                         static_cast<int>(params.size()),
                                                         static_cast<int>(constructorArgs.size()));
            }

            // Bind parameters
            for (size_t i = 0; i < params.size(); ++i)
            {
                auto paramVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    params[i].first, params[i].second, false, false);
                paramVar->setValue(constructorArgs[i]);
                environment->declareVariable(params[i].first, paramVar);
            }

            // Execute super initializer first (if present)
            if (constructor->hasSuperInitializer())
            {
                auto superInit = constructor->getSuperInitializer();
                if (superInit)
                {
                    evaluator->evaluate(superInit);
                }
            }

            // Execute constructor body if it exists
            if (constructor->getBody())
            {
                evaluator->evaluate(constructor->getBody());
            }

            environment->exitScope();
        }

        return std::static_pointer_cast<runtimeTypes::klass::ObjectInstance>(instance);
    }

    bool ScriptAPI::isObjectOfClass(const value::Value& object, const std::string& className)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            return false;
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        auto classDef = instance->getClassDefinition();
        return classDef->getName() == className;
    }

    std::string ScriptAPI::getObjectClassName(const value::Value& object)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            throw errors::TypeConversionException("Value is not an object", "unknown", "object");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        auto classDef = instance->getClassDefinition();
        return classDef->getName();
    }

    // Helper method implementations
    value::Value ScriptAPI::invokeFunction(std::shared_ptr<runtimeTypes::global::FunctionDefinition> funcDef,
                                           const std::vector<value::Value>& args)
    {
        // Set up function scope
        environment->enterScope(funcDef->getName(), environment::ScopeType::FUNCTION);

        // Bind parameters
        auto params = funcDef->getParameters();
        if (params.size() != args.size())
        {
            environment->exitScope();
            throw errors::ParameterMismatchException(funcDef->getName(),
                                                     static_cast<int>(params.size()),
                                                     static_cast<int>(args.size()));
        }

        for (size_t i = 0; i < params.size(); ++i)
        {
            auto paramVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                params[i].first, params[i].second, false, false);
            paramVar->setValue(args[i]);
            environment->declareVariable(params[i].first, paramVar);
        }

        // Execute function body
        value::Value result = std::monostate{}; // void
        if (funcDef->getBody())
        {
            result = evaluator->evaluate(funcDef->getBody().get());

            // Check if there was a return value
            if (evaluator->shouldReturn())
            {
                result = evaluator->getReturnValue();
                evaluator->setReturned(false); // Reset return state
            }
        }

        environment->exitScope();
        return result;
    }

    value::Value ScriptAPI::invokeStaticMethod(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                                               const std::string& methodName,
                                               const std::vector<value::Value>& args)
    {
        auto method = classDef->getMethod(methodName);
        if (!method || !method->isStatic())
        {
            throw errors::MethodNotFoundException(methodName, classDef->getName());
        }

        // Set up method scope
        environment->enterScope(classDef->getName() + "::" + methodName, environment::ScopeType::FUNCTION);

        try
        {
            setupStaticMethodScope(classDef, method, methodName, args);
            value::Value result = executeStaticMethodBody(method);
            environment->exitScope();
            return result;
        }
        catch (...)
        {
            // Clean up on exception
            environment->exitScope();
            throw;
        }
    }

    void ScriptAPI::setupStaticMethodScope(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                                          std::shared_ptr<runtimeTypes::klass::MethodDefinition> method,
                                          const std::string& methodName,
                                          const std::vector<value::Value>& args)
    {
        // Bind parameters
        auto params = method->getParameters();
        if (params.size() != args.size())
        {
            throw errors::ParameterMismatchException(
                classDef->getName() + "::" + methodName,
                static_cast<int>(params.size()),
                static_cast<int>(args.size()));
        }

        for (size_t i = 0; i < params.size(); ++i)
        {
            auto paramVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                params[i].first, params[i].second, false, false);
            paramVar->setValue(args[i]);
            environment->declareVariable(params[i].first, paramVar);
        }

        // Store current class name for static field access
        auto classNameVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
            "__current_class_name__", value::ValueType::STRING, classDef->getName(), false);
        environment->declareVariable("__current_class_name__", classNameVar);
    }

    value::Value ScriptAPI::executeStaticMethodBody(std::shared_ptr<runtimeTypes::klass::MethodDefinition> method)
    {
        value::Value result = std::monostate{}; // void
        if (!method->getBody())
        {
            return result;
        }

        auto apiEvaluator = std::make_unique<evaluator::Evaluator>(environment);

        // Set up method call handler for this evaluator
        auto nativeRegistry = environment->getNativeRegistry();
        if (nativeRegistry)
        {
            nativeRegistry->setMethodCallHandler(
                [&apiEvaluator](std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                const std::string& methodName,
                                const std::vector<value::Value>& args) -> value::Value
                {
                    return apiEvaluator->callMethodOnInstance(instance, methodName, args);
                }
            );
        }

        try
        {
            result = apiEvaluator->evaluate(method->getBody());

            // Check if there was a return value
            if (apiEvaluator->shouldReturn())
            {
                result = apiEvaluator->getReturnValue();
            }
        }
        catch (const errors::ReturnException& returnEx)
        {
            // Handle explicit return statements
            result = returnEx.returnValue;
        }
        catch (const std::exception& evalException)
        {
            std::cerr << "Warning: Error compiling dependency : " << evalException.what() << std::endl;
            throw;
        }

        return result;
    }
}
