#include "ScriptInterpreter.hpp"
#include "../errors/RuntimeException.hpp"
#include "../errors/ParseException.hpp"
#include "../errors/MethodNotFoundException.hpp"
#include "../errors/ObjectException.hpp"
#include "../errors/ClassNotFoundException.hpp"
#include "../errors/ParameterMismatchException.hpp"
#include "../errors/FieldNotFoundException.hpp"
#include "../errors/FinalModificationException.hpp"
#include "../errors/TypeConversionException.hpp"
#include <iostream>
#include <filesystem>
#include <memory>
#include <algorithm>

#include "ImportManager.hpp"
#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../evaluator/Evaluator.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../errors/ReturnException.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/MethodDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"

namespace services
{
    ScriptInterpreter::ScriptInterpreter()
    {
        environment::EnvironmentBuilder envBuilder;
        environment = envBuilder.build();
        evaluator = std::make_unique<evaluator::Evaluator>(environment);

        // Set up method call handler for native functions
        auto nativeRegistry = environment->getNativeRegistry();
        if (nativeRegistry)
        {
            nativeRegistry->setMethodCallHandler(
                [this](std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                       const std::string& methodName,
                       const std::vector<value::Value>& args) -> value::Value
                {
                    return evaluator->callMethodOnInstance(instance, methodName, args);
                }
            );
        }
    }

    ScriptInterpreter::~ScriptInterpreter()
    {
        // Clean up registries to prevent memory leaks in long-running programs
        cleanupRegistries();
    }

    void ScriptInterpreter::runScript(const std::string& filename)
    {
        try
        {
            // Always parse and execute .mt files directly (no auto-caching)
            lexer::Lexer lexer(filename);

            // Create and configure ImportManager
            auto importManager = std::make_unique<ImportManager>();

            // Set base directory to the directory of the script file
            std::filesystem::path scriptPath(filename);
            importManager->setBaseDirectory(scriptPath.parent_path().string());

            // Keep a raw pointer for later use before moving to Parser
            ImportManager* importManagerPtr = importManager.get();

            parser::Parser parser(lexer, std::move(importManager));

            std::unique_ptr<ASTNode> ast;
            try
            {
                ast = parser.parseProgram();
            }
            catch (const errors::ParseException&)
            {
                throw;
            }
            catch (const std::exception&)
            {
                throw;
            }
            catch (...)
            {
                throw;
            }

            // Set ImportManager on environment for clean architecture
            environment->setImportManager(importManagerPtr);

            evaluator->evaluate(ast.get());

            // Automatic cleanup after script execution to prevent memory growth
            // Only clean up interfaces that are no longer referenced
            size_t cleanedUp = cleanupUnusedInterfaces();
            if (cleanedUp > 0)
            {
                // Optional: Log cleanup activity for debugging
                // std::cout << "Cleaned up " << cleanedUp << " unused interfaces\n";
            }
        }
        catch (const ParseException&)
        {
            std::cerr << "Error" << std::endl;
            throw; // Re-throw to be caught by main()
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            throw; // Re-throw to be caught by main()
        }
    }

    void ScriptInterpreter::cleanupRegistries()
    {
        if (environment)
        {
            // Clean up unused interfaces to prevent memory growth
            environment->cleanupUnusedInterfaces();
            // Clear interface validation cache to free memory
            auto interfaceRegistry = environment->getInterfaceRegistry();
            if (interfaceRegistry)
            {
                interfaceRegistry->clearValidationCache();
            }
        }
    }

    size_t ScriptInterpreter::cleanupUnusedInterfaces()
    {
        if (!environment) return 0;
        return environment->cleanupUnusedInterfaces();
    }

    std::vector<std::string> ScriptInterpreter::findUnusedInterfaces() const
    {
        if (!environment) return {};
        return environment->findUnusedInterfaces();
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

        throw errors::MethodNotFoundException(functionName, "", __FUNCTION__);
    }

    // Method calling on objects
    Value ScriptInterpreter::callMethod(const Value& object, const std::string& methodName,
                                        const std::vector<Value>& args)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object))
        {
            throw errors::ObjectException("Cannot call method on non-object value", "", __FUNCTION__);
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
            throw errors::ClassNotFoundException(className);
        }

        return invokeStaticMethod(classDef, methodName, args);
    }


    // Static field access
    Value ScriptInterpreter::getStaticField(const std::string& className, const std::string& fieldName)
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

    void ScriptInterpreter::setStaticField(const std::string& className, const std::string& fieldName,
                                           const Value& value)
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

    // Variable access
    Value ScriptInterpreter::getVariable(const std::string& variableName)
    {
        auto varDef = environment->findVariable(variableName);
        if (!varDef)
        {
            throw errors::FieldNotFoundException(variableName);
        }

        return varDef->getValue();
    }

    void ScriptInterpreter::setVariable(const std::string& variableName, const Value& value)
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

    // Object creation
    Value ScriptInterpreter::createObject(const std::string& className, const std::vector<Value>& constructorArgs)
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
                throw errors::ParameterMismatchException("constructor", static_cast<int>(params.size()),
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
                    // Note: ScriptInterpreter uses old evaluator without context flags
                    // This will need updating when ScriptInterpreter is refactored
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
            throw errors::ParameterMismatchException(funcDef->getName(), static_cast<int>(params.size()),
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
        auto method = classDef->getMethod(methodName);
        if (!method || !method->isStatic())
        {
            throw errors::MethodNotFoundException(methodName, classDef->getName());
        }

        // Set up method scope
        environment->enterScope(classDef->getName() + "::" + methodName, environment::ScopeType::FUNCTION);

        try
        {
            // Bind parameters
            auto params = method->getParameters();
            if (params.size() != args.size())
            {
                throw errors::ParameterMismatchException(
                    classDef->getName() + "::" + methodName, static_cast<int>(params.size()),
                    static_cast<int>(args.size()));
            }

            for (size_t i = 0; i < params.size(); ++i)
            {
                auto paramVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    params[i].first, params[i].second, false, false);
                paramVar->setValue(args[i]);
                environment->declareVariable(params[i].first, paramVar);
            }

            // Store current class name for static field access (same as ObjectEvaluator does)
            auto classNameVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                "__current_class_name__", ValueType::STRING, classDef->getName(), false);
            environment->declareVariable("__current_class_name__", classNameVar);

            // Execute method body
            Value result = std::monostate{}; // void
            if (method->getBody())
            {
                auto apiEvaluator = std::make_unique<evaluator::Evaluator>(environment);

                // Set up method call handler for this evaluator too
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
                catch (const ReturnException& returnEx)
                {
                    // Handle explicit return statements - this is the normal case for static methods with return values
                    result = returnEx.returnValue;
                }
                catch (const std::exception& evalException)
                {
                    std::cerr << "Warning: Error compiling dependency : " << evalException.what() << std::endl;
                    throw;
                }
                catch (...)
                {
                    throw;
                }
            }
            // Clean up and return
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
            throw errors::TypeConversionException("Value is not an object", "unknown", "object");
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        auto classDef = instance->getClassDefinition();
        return classDef->getName();
    }

    void ScriptInterpreter::preRegisterClassDefinitions(ast::ASTNode* node)
    {
        if (!node) return;

        // Check if this node is a ClassNode
        if (auto classNode = dynamic_cast<ast::ClassNode*>(node))
        {
            // Pre-register class in environment

            // Evaluate the ClassNode to register it in the environment
            evaluator->evaluate(classNode);
            return; // No need to traverse children of ClassNode
        }

        // Recursively process child nodes
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                preRegisterClassDefinitions(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                preRegisterClassDefinitions(statement.get());
            }
        }
        // Add other node types that may contain ClassNodes as needed
    }
}
