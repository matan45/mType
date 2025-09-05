#include "ScriptInterpreter.hpp"
#include "ImportManager.hpp"
#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../evaluator/Evaluator.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../exception/ReturnException.hpp"
#include <iostream>
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/MethodDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include <stdexcept>
#include <memory>
#include <filesystem>

namespace services
{
    ScriptInterpreter::ScriptInterpreter()
    {
        environment::EnvironmentBuilder envBuilder;
        environment = envBuilder.build();
        evaluator = std::make_unique<evaluator::Evaluator>(environment);
    }

    ScriptInterpreter::~ScriptInterpreter() = default;

    void ScriptInterpreter::runScript(const std::string& filename)
    {
        // Parse and execute
        lexer::Lexer lexer(filename);

        // Create and configure ImportManager
        auto importManager = std::make_unique<ImportManager>();

        // Set base directory to the directory of the script file
        std::filesystem::path scriptPath(filename);
        importManager->setBaseDirectory(scriptPath.parent_path().string());

        // Keep a raw pointer for later use before moving to Parser
        ImportManager* importManagerPtr = importManager.get();
        parser::Parser parser(lexer, std::move(importManager));
        auto ast = parser.parseProgram();

        // Set ImportManager on environment for clean architecture
        environment->setImportManager(importManagerPtr);

        evaluator->evaluate(ast.get());
    }


    void ScriptInterpreter::registerNativeFunction(const std::string& name, NativeFunction function)
    {
        auto nativeRegistry = environment->getNativeRegistry();
        nativeRegistry->registerNativeFunction(name, function);
    }

    void ScriptInterpreter::registerNativeVariable(const std::string& name, const Value& value)
    {
        // Create a variable definition and register it
        auto varDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
            name, getValueType(value), false, false);
        varDef->setValue(value);

        environment->declareVariable(name, varDef);
    }

    void ScriptInterpreter::registerNativeClass(const std::string& name)
    {
        // Create a basic class definition for native classes
        auto classDef = std::make_shared<runtimeTypes::klass::ClassDefinition>(name);

        environment->registerClass(name, classDef);
    }

    void ScriptInterpreter::registerNativeMethod(const std::string& className,
                                                 const std::string& methodName,
                                                 NativeFunction function,
                                                 bool isStatic)
    {
        auto nativeRegistry = environment->getNativeRegistry();
        nativeRegistry->registerNativeFunction(methodName, function);
    }

    void ScriptInterpreter::registerNativeField(const std::string& className,
                                                const std::string& fieldName,
                                                const value::Value& value,
                                                bool isStatic)
    {
        registerNativeVariable(fieldName, value);
    }

    // C++ to mType calling interface implementations

    // Function calling
    Value ScriptInterpreter::callFunction(const std::string& functionName, const std::vector<Value>& args)
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

        throw std::runtime_error("Function not found: " + functionName);
    }

    // Method calling on objects
    Value ScriptInterpreter::callMethod(const Value& object, const std::string& methodName,
                                        const std::vector<Value>& args)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            throw std::runtime_error("Cannot call method on non-object value");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        return evaluator->callMethodOnInstance(instance, methodName, args);
    }

    // Static method calling
    Value ScriptInterpreter::callStaticMethod(const std::string& className, const std::string& methodName,
                                              const std::vector<Value>& args)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw std::runtime_error("Class not found: " + className);
        }

        return invokeStaticMethod(classDef, methodName, args);
    }


    // Static field access
    Value ScriptInterpreter::getStaticField(const std::string& className, const std::string& fieldName)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw std::runtime_error("Class not found: " + className);
        }

        auto field = classDef->getField(fieldName);
        if (!field || !field->isStatic())
        {
            throw std::runtime_error("Static field not found: " + className + "::" + fieldName);
        }

        return field->getValue();
    }

    void ScriptInterpreter::setStaticField(const std::string& className, const std::string& fieldName,
                                           const Value& value)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw std::runtime_error("Class not found: " + className);
        }

        auto field = classDef->getField(fieldName);
        if (!field || !field->isStatic())
        {
            throw std::runtime_error("Static field not found: " + className + "::" + fieldName);
        }

        if (field->isFinal())
        {
            throw std::runtime_error("Cannot modify final field: " + className + "::" + fieldName);
        }

        field->setValue(value);
    }

    // Variable access
    Value ScriptInterpreter::getVariable(const std::string& variableName)
    {
        auto varDef = environment->findVariable(variableName);
        if (!varDef)
        {
            throw std::runtime_error("Variable not found: " + variableName);
        }

        return varDef->getValue();
    }

    void ScriptInterpreter::setVariable(const std::string& variableName, const Value& value)
    {
        auto varDef = environment->findVariable(variableName);
        if (!varDef)
        {
            throw std::runtime_error("Variable not found: " + variableName);
        }

        if (varDef->isFinal())
        {
            throw std::runtime_error("Cannot modify final variable: " + variableName);
        }

        varDef->setValue(value);
    }

    // Object creation
    Value ScriptInterpreter::createObject(const std::string& className, const std::vector<Value>& constructorArgs)
    {
        auto classDef = environment->findClass(className);
        if (!classDef)
        {
            throw std::runtime_error("Class not found: " + className);
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
                throw std::runtime_error("Constructor parameter count mismatch");
            }

            // Bind parameters
            for (size_t i = 0; i < params.size(); ++i)
            {
                auto paramVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    params[i].first, params[i].second, false, false);
                paramVar->setValue(constructorArgs[i]);
                environment->declareVariable(params[i].first, paramVar);
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

    Value ScriptInterpreter::createObjectForReturn(const std::string& className,
        const std::vector<Value>& constructorArgs)
    {
        // Use the existing createObject method - it already handles object creation properly
        return createObject(className, constructorArgs);
    }

    // Helper method implementations
    Value ScriptInterpreter::invokeFunction(std::shared_ptr<runtimeTypes::global::FunctionDefinition> funcDef,
                                            const std::vector<Value>& args)
    {
        // Set up function scope
        environment->enterScope(funcDef->getName(), environment::ScopeType::FUNCTION);

        // Bind parameters
        auto params = funcDef->getParameters();
        if (params.size() != args.size())
        {
            environment->exitScope();
            throw std::runtime_error("Function parameter count mismatch for " + funcDef->getName());
        }

        for (size_t i = 0; i < params.size(); ++i)
        {
            auto paramVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                params[i].first, params[i].second, false, false);
            paramVar->setValue(args[i]);
            environment->declareVariable(params[i].first, paramVar);
        }

        // Execute function body
        Value result = std::monostate{}; // void
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

    Value ScriptInterpreter::invokeStaticMethod(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                                                const std::string& methodName,
                                                const std::vector<Value>& args)
    {
        std::cout << "[DEBUG] invokeStaticMethod called: " << classDef->getName() << "::" << methodName << std::endl;
        
        auto method = classDef->getMethod(methodName);
        if (!method || !method->isStatic())
        {
            throw std::runtime_error("Static method not found: " + classDef->getName() + "::" + methodName);
        }

        std::cout << "[DEBUG] Method found, entering scope..." << std::endl;
        // Set up method scope
        environment->enterScope(classDef->getName() + "::" + methodName, environment::ScopeType::FUNCTION);

        try
        {
            std::cout << "[DEBUG] Binding parameters..." << std::endl;
            // Bind parameters
            auto params = method->getParameters();
            if (params.size() != args.size())
            {
                throw std::runtime_error(
                    "Method parameter count mismatch for " + classDef->getName() + "::" + methodName);
            }

            for (size_t i = 0; i < params.size(); ++i)
            {
                auto paramVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    params[i].first, params[i].second, false, false);
                paramVar->setValue(args[i]);
                environment->declareVariable(params[i].first, paramVar);
            }

            std::cout << "[DEBUG] Starting method execution..." << std::endl;
            
            // Store current class name for static field access (same as ObjectEvaluator does)
            auto classNameVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                "__current_class_name__", ValueType::STRING, classDef->getName(), false);
            environment->declareVariable("__current_class_name__", classNameVar);
            
            // Execute method body
            Value result = std::monostate{}; // void
            if (method->getBody())
            {
                std::cout << "[DEBUG] Method has body, evaluating..." << std::endl;
                std::cout << "[DEBUG] Method body AST type: " << typeid(*method->getBody()).name() << std::endl;
                
                // Create a fresh evaluator instance for API calls to avoid reentrancy issues
                std::cout << "[DEBUG] Creating fresh evaluator for API call..." << std::endl;
                auto apiEvaluator = std::make_unique<evaluator::Evaluator>(environment);
                
                try
                {
                    std::cout << "[DEBUG] About to call apiEvaluator->evaluate()..." << std::endl;
                    result = apiEvaluator->evaluate(method->getBody());
                    std::cout << "[DEBUG] apiEvaluator->evaluate() completed" << std::endl;

                    // Check if there was a return value
                    if (apiEvaluator->shouldReturn())
                    {
                        std::cout << "[DEBUG] Getting return value..." << std::endl;
                        result = apiEvaluator->getReturnValue();
                    }
                }
                catch (const exception::ReturnException& returnEx)
                {
                    std::cout << "[DEBUG] Caught ReturnException" << std::endl;
                    // Handle explicit return statements - this is the normal case for static methods with return values
                    result = returnEx.returnValue;
                }
                catch (const std::exception& evalException)
                {
                    std::cout << "[DEBUG] Exception during API evaluation: " << evalException.what() << std::endl;
                    throw;
                }
                catch (...)
                {
                    std::cout << "[DEBUG] Unknown exception during API evaluation" << std::endl;
                    throw;
                }
            }

            std::cout << "[DEBUG] Method execution completed, cleaning up..." << std::endl;
            // Clean up and return
            environment->exitScope();

            std::cout << "[DEBUG] Returning result from invokeStaticMethod" << std::endl;
            return result;
        }
        catch (...)
        {
            std::cout << "[DEBUG] Exception in invokeStaticMethod, cleaning up..." << std::endl;
            // Clean up on exception
            environment->exitScope();
            throw;
        }
    }


    // Utility methods for native functions working with objects
    bool ScriptInterpreter::isObjectOfClass(const Value& object, const std::string& className)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            return false;
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        auto classDef = instance->getClassDefinition();
        return classDef->getName() == className;
    }

    std::string ScriptInterpreter::getObjectClassName(const Value& object)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            throw std::runtime_error("Value is not an object");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        auto classDef = instance->getClassDefinition();
        return classDef->getName();
    }
}
