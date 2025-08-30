#pragma once
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"

// Forward declarations
namespace evaluator { class Evaluator; }

namespace services
{
    using NativeFunction = std::function<value::Value(const std::vector<value::Value>&)>;
    
    class ScriptInterpreter
    {
    private:
        std::shared_ptr<environment::Environment> environment;
        std::unique_ptr<evaluator::Evaluator> evaluator;
        
        // Helper methods for internal use
        value::Value invokeFunction(std::shared_ptr<runtimeTypes::global::FunctionDefinition> funcDef, const std::vector<value::Value>& args);
        value::Value invokeStaticMethod(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef, const std::string& methodName, const std::vector<value::Value>& args);
        
    public:
        ScriptInterpreter();
        ~ScriptInterpreter();
        void runScript(const std::string& filename);
        void runScriptContent(const std::string& content, const std::string& filename = "inline");
        
        // Native function registration
        void registerNativeFunction(const std::string& name, NativeFunction function);
        void registerNativeVariable(const std::string& name, const value::Value& value);
        
        // Native class support
        void registerNativeClass(const std::string& name, 
                                const std::string& namespaceName = "");
        void registerNativeMethod(const std::string& className, 
                                 const std::string& methodName, 
                                 NativeFunction function,
                                 bool isStatic = false);
        void registerNativeField(const std::string& className,
                                const std::string& fieldName, 
                                const value::Value& value,
                                bool isStatic = false);
        
        // C++ to mType calling interface
        // Function calling
        value::Value callFunction(const std::string& functionName, const std::vector<value::Value>& args = {});
        value::Value callQualifiedFunction(const std::vector<std::string>& namespacePath, const std::string& functionName, const std::vector<value::Value>& args = {});
        
        // Method calling on objects
        value::Value callMethod(const value::Value& object, const std::string& methodName, const std::vector<value::Value>& args = {});
        
        // Static method calling
        value::Value callStaticMethod(const std::string& className, const std::string& methodName, const std::vector<value::Value>& args = {});
        value::Value callQualifiedStaticMethod(const std::vector<std::string>& namespacePath, const std::string& className, const std::string& methodName, const std::vector<value::Value>& args = {});
        
        // Static field access
        value::Value getStaticField(const std::string& className, const std::string& fieldName);
        void setStaticField(const std::string& className, const std::string& fieldName, const value::Value& value);
        value::Value getQualifiedStaticField(const std::vector<std::string>& namespacePath, const std::string& className, const std::string& fieldName);
        void setQualifiedStaticField(const std::vector<std::string>& namespacePath, const std::string& className, const std::string& fieldName, const value::Value& value);
        
        // Variable access
        value::Value getVariable(const std::string& variableName);
        void setVariable(const std::string& variableName, const value::Value& value);
        value::Value getQualifiedVariable(const std::vector<std::string>& namespacePath, const std::string& variableName);
        void setQualifiedVariable(const std::vector<std::string>& namespacePath, const std::string& variableName, const value::Value& value);
        
        // Object creation and manipulation
        value::Value createObject(const std::string& className, const std::vector<value::Value>& constructorArgs = {});
        value::Value createQualifiedObject(const std::vector<std::string>& namespacePath, const std::string& className, const std::vector<value::Value>& constructorArgs = {});
        
        // Native function object creation helpers (for use inside native functions)
        value::Value createNamespacedObject(const std::vector<std::string>& namespacePath, const std::string& className, const std::vector<value::Value>& constructorArgs = {});
        value::Value createObjectForReturn(const std::string& qualifiedClassName, const std::vector<value::Value>& constructorArgs = {});
        
        // Utility methods for native functions working with objects
        bool isObjectOfClass(const value::Value& object, const std::string& className);
        bool isObjectOfQualifiedClass(const value::Value& object, const std::vector<std::string>& namespacePath, const std::string& className);
        std::string getObjectClassName(const value::Value& object);
        std::vector<std::string> getObjectQualifiedClassName(const value::Value& object);
        
        // Helper to parse qualified class names (e.g. "math::Vector" -> {"math"}, "Vector")
        std::pair<std::vector<std::string>, std::string> parseQualifiedClassName(const std::string& qualifiedClassName);
        
        // Native function registration with namespace support
        void registerNativeClassInNamespace(const std::vector<std::string>& namespacePath, const std::string& className);
        void registerNativeMethodInNamespace(const std::vector<std::string>& namespacePath, const std::string& className, 
                                            const std::string& methodName, NativeFunction function, bool isStatic = false);
        void registerNativeFieldInNamespace(const std::vector<std::string>& namespacePath, const std::string& className,
                                           const std::string& fieldName, const value::Value& value, bool isStatic = false);
        
        // Access to environment and evaluator for advanced operations
        std::shared_ptr<environment::Environment> getEnvironment() const { return environment; }
        evaluator::Evaluator* getEvaluator() const { return evaluator.get(); }
    };
}

