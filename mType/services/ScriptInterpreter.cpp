#include "ScriptInterpreter.hpp"
#include "ImportManager.hpp"
#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../evaluator/Evaluator.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/MethodDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
#include "../ast/nodes/functions/FunctionCallNode.hpp"
#include "../ast/nodes/classes/MethodCallNode.hpp"
#include <iostream>
#include <fstream>
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
        // Read the script file
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filename);
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        runScriptContent(content, filename);
    }
    
    void ScriptInterpreter::runScriptContent(const std::string& content, const std::string& filename)
    {
        // Parse and execute
        lexer::Lexer lexer(content, filename);
        
        // Create and configure ImportManager
        auto importManager = std::make_shared<ImportManager>();
        
        // Set base directory to the directory of the script file
        std::filesystem::path scriptPath(filename);
        importManager->setBaseDirectory(scriptPath.parent_path().string());
        
        parser::Parser parser(lexer);
        auto ast = parser.parseProgram();
        
        // Set ImportManager on environment for clean architecture
        environment->setImportManager(importManager.get());
        
        evaluator->evaluate(ast.get());
    }
    
    void ScriptInterpreter::registerNativeFunction(const std::string& name, NativeFunction function)
    {
        auto nativeRegistry = environment->getNativeRegistry();
        nativeRegistry->registerNativeFunction(name, function);
    }
    
    void ScriptInterpreter::registerNativeVariable(const std::string& name, const value::Value& value)
    {
        // Create a variable definition and register it
        auto varDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
            name, value::getValueType(value), false, false);
        varDef->setValue(value);
        
        environment->declareVariable(name, varDef);
    }
    
    void ScriptInterpreter::registerNativeClass(const std::string& name, const std::string& namespaceName)
    {
        // Create a basic class definition for native classes
        auto classDef = std::make_shared<runtimeTypes::klass::ClassDefinition>(name);
        
        // Note: ClassDefinition doesn't have setNamespace method in current implementation
        // For now, we'll register it directly without namespace support
        
        environment->registerClass(name, classDef);
    }
    
    void ScriptInterpreter::registerNativeMethod(const std::string& className, 
                                                const std::string& methodName, 
                                                NativeFunction function,
                                                bool isStatic)
    {
        // Register as a native function with qualified name
        std::string qualifiedName = className + "::" + methodName;
        if (isStatic) {
            qualifiedName = className + "::" + methodName + "::static";
        }
        
        auto nativeRegistry = environment->getNativeRegistry();
        nativeRegistry->registerNativeFunction(qualifiedName, function);
    }
    
    void ScriptInterpreter::registerNativeField(const std::string& className,
                                               const std::string& fieldName, 
                                               const value::Value& value,
                                               bool isStatic)
    {
        // Register as a native variable with qualified name
        std::string qualifiedName = className + "::" + fieldName;
        if (isStatic) {
            qualifiedName = className + "::" + fieldName + "::static";
        }
        
        registerNativeVariable(qualifiedName, value);
    }
    
    // C++ to mType calling interface implementations
    
    // Function calling
    value::Value ScriptInterpreter::callFunction(const std::string& functionName, const std::vector<value::Value>& args)
    {
        // First try to find a regular mType function
        auto funcDef = environment->findFunction(functionName);
        if (funcDef) {
            return invokeFunction(funcDef, args);
        }
        
        // If not found, try to find a native function
        auto nativeRegistry = environment->getNativeRegistry();
        if (nativeRegistry && nativeRegistry->hasNativeFunction(functionName)) {
            auto nativeFunc = nativeRegistry->findNativeFunction(functionName);
            if (nativeFunc) {
                return nativeFunc(args);
            }
        }
        
        throw std::runtime_error("Function not found: " + functionName);
    }
    
    value::Value ScriptInterpreter::callQualifiedFunction(const std::vector<std::string>& namespacePath, const std::string& functionName, const std::vector<value::Value>& args)
    {
        auto funcRegistry = environment->getFunctionRegistry();
        auto funcDef = funcRegistry->findFunctionInNamespace(namespacePath, functionName);
        if (!funcDef) {
            std::string qualifiedName = "";
            for (const auto& part : namespacePath) {
                qualifiedName += part + "::";
            }
            qualifiedName += functionName;
            throw std::runtime_error("Qualified function not found: " + qualifiedName);
        }
        
        return invokeFunction(funcDef, args);
    }
    
    // Method calling on objects
    value::Value ScriptInterpreter::callMethod(const value::Value& object, const std::string& methodName, const std::vector<value::Value>& args)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object)) {
            throw std::runtime_error("Cannot call method on non-object value");
        }
        
        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        return evaluator->callMethodOnInstance(instance, methodName, args);
    }
    
    // Static method calling
    value::Value ScriptInterpreter::callStaticMethod(const std::string& className, const std::string& methodName, const std::vector<value::Value>& args)
    {
        auto classDef = environment->findClass(className);
        if (!classDef) {
            throw std::runtime_error("Class not found: " + className);
        }
        
        return invokeStaticMethod(classDef, methodName, args);
    }
    
    value::Value ScriptInterpreter::callQualifiedStaticMethod(const std::vector<std::string>& namespacePath, const std::string& className, const std::string& methodName, const std::vector<value::Value>& args)
    {
        auto classRegistry = environment->getClassRegistry();
        auto classDef = classRegistry->findClassInNamespace(namespacePath, className);
        if (!classDef) {
            std::string qualifiedName = "";
            for (const auto& part : namespacePath) {
                qualifiedName += part + "::";
            }
            qualifiedName += className;
            throw std::runtime_error("Qualified class not found: " + qualifiedName);
        }
        
        return invokeStaticMethod(classDef, methodName, args);
    }
    
    // Static field access
    value::Value ScriptInterpreter::getStaticField(const std::string& className, const std::string& fieldName)
    {
        auto classDef = environment->findClass(className);
        if (!classDef) {
            throw std::runtime_error("Class not found: " + className);
        }
        
        auto field = classDef->getField(fieldName);
        if (!field || !field->isStatic()) {
            throw std::runtime_error("Static field not found: " + className + "::" + fieldName);
        }
        
        return field->getValue();
    }
    
    void ScriptInterpreter::setStaticField(const std::string& className, const std::string& fieldName, const value::Value& value)
    {
        auto classDef = environment->findClass(className);
        if (!classDef) {
            throw std::runtime_error("Class not found: " + className);
        }
        
        auto field = classDef->getField(fieldName);
        if (!field || !field->isStatic()) {
            throw std::runtime_error("Static field not found: " + className + "::" + fieldName);
        }
        
        if (field->isFinal()) {
            throw std::runtime_error("Cannot modify final field: " + className + "::" + fieldName);
        }
        
        field->setValue(value);
    }
    
    value::Value ScriptInterpreter::getQualifiedStaticField(const std::vector<std::string>& namespacePath, const std::string& className, const std::string& fieldName)
    {
        auto classRegistry = environment->getClassRegistry();
        auto classDef = classRegistry->findClassInNamespace(namespacePath, className);
        if (!classDef) {
            std::string qualifiedName = "";
            for (const auto& part : namespacePath) {
                qualifiedName += part + "::";
            }
            qualifiedName += className;
            throw std::runtime_error("Qualified class not found: " + qualifiedName);
        }
        
        auto field = classDef->getField(fieldName);
        if (!field || !field->isStatic()) {
            std::string qualifiedFieldName = "";
            for (const auto& part : namespacePath) {
                qualifiedFieldName += part + "::";
            }
            qualifiedFieldName += className + "::" + fieldName;
            throw std::runtime_error("Static field not found: " + qualifiedFieldName);
        }
        
        return field->getValue();
    }
    
    void ScriptInterpreter::setQualifiedStaticField(const std::vector<std::string>& namespacePath, const std::string& className, const std::string& fieldName, const value::Value& value)
    {
        auto classRegistry = environment->getClassRegistry();
        auto classDef = classRegistry->findClassInNamespace(namespacePath, className);
        if (!classDef) {
            std::string qualifiedName = "";
            for (const auto& part : namespacePath) {
                qualifiedName += part + "::";
            }
            qualifiedName += className;
            throw std::runtime_error("Qualified class not found: " + qualifiedName);
        }
        
        auto field = classDef->getField(fieldName);
        if (!field || !field->isStatic()) {
            std::string qualifiedFieldName = "";
            for (const auto& part : namespacePath) {
                qualifiedFieldName += part + "::";
            }
            qualifiedFieldName += className + "::" + fieldName;
            throw std::runtime_error("Static field not found: " + qualifiedFieldName);
        }
        
        if (field->isFinal()) {
            std::string qualifiedFieldName = "";
            for (const auto& part : namespacePath) {
                qualifiedFieldName += part + "::";
            }
            qualifiedFieldName += className + "::" + fieldName;
            throw std::runtime_error("Cannot modify final field: " + qualifiedFieldName);
        }
        
        field->setValue(value);
    }
    
    // Variable access
    value::Value ScriptInterpreter::getVariable(const std::string& variableName)
    {
        auto varDef = environment->findVariable(variableName);
        if (!varDef) {
            throw std::runtime_error("Variable not found: " + variableName);
        }
        
        return varDef->getValue();
    }
    
    void ScriptInterpreter::setVariable(const std::string& variableName, const value::Value& value)
    {
        auto varDef = environment->findVariable(variableName);
        if (!varDef) {
            throw std::runtime_error("Variable not found: " + variableName);
        }
        
        if (varDef->isFinal()) {
            throw std::runtime_error("Cannot modify final variable: " + variableName);
        }
        
        varDef->setValue(value);
    }
    
    value::Value ScriptInterpreter::getQualifiedVariable(const std::vector<std::string>& namespacePath, const std::string& variableName)
    {
        // For qualified variables, we need to construct the qualified name
        // This depends on how variables are stored in the environment
        // For now, let's use a simple approach
        std::string qualifiedName = "";
        for (const auto& part : namespacePath) {
            qualifiedName += part + "::";
        }
        qualifiedName += variableName;
        
        auto varDef = environment->findVariable(qualifiedName);
        if (!varDef) {
            throw std::runtime_error("Qualified variable not found: " + qualifiedName);
        }
        
        return varDef->getValue();
    }
    
    void ScriptInterpreter::setQualifiedVariable(const std::vector<std::string>& namespacePath, const std::string& variableName, const value::Value& value)
    {
        std::string qualifiedName = "";
        for (const auto& part : namespacePath) {
            qualifiedName += part + "::";
        }
        qualifiedName += variableName;
        
        auto varDef = environment->findVariable(qualifiedName);
        if (!varDef) {
            throw std::runtime_error("Qualified variable not found: " + qualifiedName);
        }
        
        if (varDef->isFinal()) {
            throw std::runtime_error("Cannot modify final variable: " + qualifiedName);
        }
        
        varDef->setValue(value);
    }
    
    // Object creation
    value::Value ScriptInterpreter::createObject(const std::string& className, const std::vector<value::Value>& constructorArgs)
    {
        auto classDef = environment->findClass(className);
        if (!classDef) {
            throw std::runtime_error("Class not found: " + className);
        }
        
        // Create the object instance
        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);
        
        // Find and call appropriate constructor
        auto constructor = classDef->findConstructor(constructorArgs.size());
        if (constructor) {
            // Set up parameters in a new scope
            environment->enterScope("constructor");
            
            auto params = constructor->getParameters();
            if (params.size() != constructorArgs.size()) {
                environment->exitScope();
                throw std::runtime_error("Constructor parameter count mismatch");
            }
            
            // Bind parameters
            for (size_t i = 0; i < params.size(); ++i) {
                auto paramVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    params[i].first, params[i].second, false, false);
                paramVar->setValue(constructorArgs[i]);
                environment->declareVariable(params[i].first, paramVar);
            }
            
            // Execute constructor body if it exists
            if (constructor->getBody()) {
                evaluator->evaluate(constructor->getBody());
            }
            
            environment->exitScope();
        }
        
        return std::static_pointer_cast<runtimeTypes::klass::ObjectInstance>(instance);
    }
    
    value::Value ScriptInterpreter::createQualifiedObject(const std::vector<std::string>& namespacePath, const std::string& className, const std::vector<value::Value>& constructorArgs)
    {
        auto classRegistry = environment->getClassRegistry();
        auto classDef = classRegistry->findClassInNamespace(namespacePath, className);
        if (!classDef) {
            std::string qualifiedName = "";
            for (const auto& part : namespacePath) {
                qualifiedName += part + "::";
            }
            qualifiedName += className;
            throw std::runtime_error("Qualified class not found: " + qualifiedName);
        }
        
        // Create the object instance
        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);
        
        // Find and call appropriate constructor
        auto constructor = classDef->findConstructor(constructorArgs.size());
        if (constructor) {
            // Set up parameters in a new scope
            environment->enterScope("constructor");
            
            auto params = constructor->getParameters();
            if (params.size() != constructorArgs.size()) {
                environment->exitScope();
                throw std::runtime_error("Constructor parameter count mismatch");
            }
            
            // Bind parameters
            for (size_t i = 0; i < params.size(); ++i) {
                auto paramVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                    params[i].first, params[i].second, false, false);
                paramVar->setValue(constructorArgs[i]);
                environment->declareVariable(params[i].first, paramVar);
            }
            
            // Execute constructor body if it exists
            if (constructor->getBody()) {
                evaluator->evaluate(constructor->getBody());
            }
            
            environment->exitScope();
        }
        
        return std::static_pointer_cast<runtimeTypes::klass::ObjectInstance>(instance);
    }
    
    // Helper method implementations
    value::Value ScriptInterpreter::invokeFunction(std::shared_ptr<runtimeTypes::global::FunctionDefinition> funcDef, const std::vector<value::Value>& args)
    {
        // Set up function scope
        environment->enterScope(funcDef->getName(), environment::ScopeType::FUNCTION);
        
        // Bind parameters
        auto params = funcDef->getParameters();
        if (params.size() != args.size()) {
            environment->exitScope();
            throw std::runtime_error("Function parameter count mismatch for " + funcDef->getName());
        }
        
        for (size_t i = 0; i < params.size(); ++i) {
            auto paramVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                params[i].first, params[i].second, false, false);
            paramVar->setValue(args[i]);
            environment->declareVariable(params[i].first, paramVar);
        }
        
        // Execute function body
        value::Value result = std::monostate{}; // void
        if (funcDef->getBody()) {
            result = evaluator->evaluate(funcDef->getBody());
            
            // Check if there was a return value
            if (evaluator->shouldReturn()) {
                result = evaluator->getReturnValue();
                evaluator->setReturned(false); // Reset return state
            }
        }
        
        environment->exitScope();
        return result;
    }
    
    value::Value ScriptInterpreter::invokeStaticMethod(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef, const std::string& methodName, const std::vector<value::Value>& args)
    {
        auto method = classDef->getMethod(methodName);
        if (!method || !method->isStatic()) {
            throw std::runtime_error("Static method not found: " + classDef->getName() + "::" + methodName);
        }
        
        // Set up method scope
        environment->enterScope(classDef->getName() + "::" + methodName, environment::ScopeType::FUNCTION);
        
        // Bind parameters
        auto params = method->getParameters();
        if (params.size() != args.size()) {
            environment->exitScope();
            throw std::runtime_error("Method parameter count mismatch for " + classDef->getName() + "::" + methodName);
        }
        
        for (size_t i = 0; i < params.size(); ++i) {
            auto paramVar = std::make_shared<runtimeTypes::global::VariableDefinition>(
                params[i].first, params[i].second, false, false);
            paramVar->setValue(args[i]);
            environment->declareVariable(params[i].first, paramVar);
        }
        
        // Execute method body
        value::Value result = std::monostate{}; // void
        if (method->getBody()) {
            result = evaluator->evaluate(method->getBody());
            
            // Check if there was a return value
            if (evaluator->shouldReturn()) {
                result = evaluator->getReturnValue();
                evaluator->setReturned(false); // Reset return state
            }
        }
        
        environment->exitScope();
        return result;
    }
    
    // Native function object creation helpers
    value::Value ScriptInterpreter::createNamespacedObject(const std::vector<std::string>& namespacePath, const std::string& className, const std::vector<value::Value>& constructorArgs)
    {
        return createQualifiedObject(namespacePath, className, constructorArgs);
    }
    
    value::Value ScriptInterpreter::createObjectForReturn(const std::string& qualifiedClassName, const std::vector<value::Value>& constructorArgs)
    {
        auto parsedName = parseQualifiedClassName(qualifiedClassName);
        if (parsedName.first.empty()) {
            // Simple class name
            return createObject(parsedName.second, constructorArgs);
        } else {
            // Qualified class name
            return createQualifiedObject(parsedName.first, parsedName.second, constructorArgs);
        }
    }
    
    // Utility methods for native functions working with objects
    bool ScriptInterpreter::isObjectOfClass(const value::Value& object, const std::string& className)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object)) {
            return false;
        }
        
        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        auto classDef = instance->getClassDefinition();
        return classDef->getName() == className;
    }
    
    bool ScriptInterpreter::isObjectOfQualifiedClass(const value::Value& object, const std::vector<std::string>& namespacePath, const std::string& className)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object)) {
            return false;
        }
        
        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        auto classDef = instance->getClassDefinition();
        
        // Check if class name matches
        if (classDef->getName() != className) {
            return false;
        }
        
        // For now, we'll do a simple check - this would need to be enhanced
        // based on how namespace information is stored in ClassDefinition
        // This is a simplified implementation
        return true; // Would need proper namespace comparison
    }
    
    std::string ScriptInterpreter::getObjectClassName(const value::Value& object)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object)) {
            throw std::runtime_error("Value is not an object");
        }
        
        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        auto classDef = instance->getClassDefinition();
        return classDef->getName();
    }
    
    std::vector<std::string> ScriptInterpreter::getObjectQualifiedClassName(const value::Value& object)
    {
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object)) {
            throw std::runtime_error("Value is not an object");
        }
        
        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(object);
        auto classDef = instance->getClassDefinition();
        
        // For now, return just the class name as a single element
        // This would need to be enhanced with proper namespace support
        return {classDef->getName()};
    }
    
    // Helper to parse qualified class names
    std::pair<std::vector<std::string>, std::string> ScriptInterpreter::parseQualifiedClassName(const std::string& qualifiedClassName)
    {
        std::vector<std::string> namespaceParts;
        std::string className;
        
        size_t lastDelimiter = qualifiedClassName.rfind("::");
        if (lastDelimiter == std::string::npos) {
            // No namespace, just class name
            className = qualifiedClassName;
        } else {
            // Has namespace
            className = qualifiedClassName.substr(lastDelimiter + 2);
            std::string namespaceStr = qualifiedClassName.substr(0, lastDelimiter);
            
            // Split namespace by "::"
            size_t start = 0;
            size_t end = namespaceStr.find("::");
            
            while (end != std::string::npos) {
                namespaceParts.push_back(namespaceStr.substr(start, end - start));
                start = end + 2;
                end = namespaceStr.find("::", start);
            }
            namespaceParts.push_back(namespaceStr.substr(start));
        }
        
        return {namespaceParts, className};
    }
    
    // Native function registration with namespace support
    void ScriptInterpreter::registerNativeClassInNamespace(const std::vector<std::string>& namespacePath, const std::string& className)
    {
        // Create a basic class definition for native classes in namespace
        auto classDef = std::make_shared<runtimeTypes::klass::ClassDefinition>(className);
        
        // Register the class in the namespace
        auto classRegistry = environment->getClassRegistry();
        classRegistry->registerClassInNamespace(namespacePath, className, classDef);
    }
    
    void ScriptInterpreter::registerNativeMethodInNamespace(const std::vector<std::string>& namespacePath, const std::string& className, 
                                                          const std::string& methodName, NativeFunction function, bool isStatic)
    {
        // Build qualified name for namespace
        std::string qualifiedName = "";
        for (const auto& part : namespacePath) {
            qualifiedName += part + "::";
        }
        qualifiedName += className + "::" + methodName;
        
        if (isStatic) {
            qualifiedName += "::static";
        }
        
        auto nativeRegistry = environment->getNativeRegistry();
        nativeRegistry->registerNativeFunction(qualifiedName, function);
    }
    
    void ScriptInterpreter::registerNativeFieldInNamespace(const std::vector<std::string>& namespacePath, const std::string& className,
                                                          const std::string& fieldName, const value::Value& value, bool isStatic)
    {
        // Build qualified name for namespace
        std::string qualifiedName = "";
        for (const auto& part : namespacePath) {
            qualifiedName += part + "::";
        }
        qualifiedName += className + "::" + fieldName;
        
        if (isStatic) {
            qualifiedName += "::static";
        }
        
        registerNativeVariable(qualifiedName, value);
    }
}
