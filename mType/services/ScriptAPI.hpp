#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"

namespace evaluator
{
    class Evaluator;
}

namespace services
{
    /**
     * Service providing C++ to mType calling interface
     * Allows C++ code to call mType functions, methods, and manipulate objects
     */
    class ScriptAPI
    {
    private:
        std::shared_ptr<environment::Environment> environment;
        evaluator::Evaluator* evaluator;

        // Helper methods for internal use
        value::Value invokeFunction(std::shared_ptr<runtimeTypes::global::FunctionDefinition> funcDef,
                                    const std::vector<value::Value>& args);
        value::Value invokeStaticMethod(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                                        const std::string& methodName,
                                        const std::vector<value::Value>& args);

        // Static method execution helpers
        void setupStaticMethodScope(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                                   std::shared_ptr<runtimeTypes::klass::MethodDefinition> method,
                                   const std::string& methodName,
                                   const std::vector<value::Value>& args);
        value::Value executeStaticMethodBody(std::shared_ptr<runtimeTypes::klass::MethodDefinition> method);

    public:
        ScriptAPI(std::shared_ptr<environment::Environment> env, evaluator::Evaluator* eval);
        ~ScriptAPI();

        // Function calling
        value::Value callFunction(const std::string& functionName,
                                 const std::vector<value::Value>& args = {});

        // Method calling on objects
        value::Value callMethod(const value::Value& object,
                               const std::string& methodName,
                               const std::vector<value::Value>& args = {});

        // Static method calling
        value::Value callStaticMethod(const std::string& className,
                                     const std::string& methodName,
                                     const std::vector<value::Value>& args = {});

        // Static field access
        value::Value getStaticField(const std::string& className, const std::string& fieldName);
        void setStaticField(const std::string& className,
                           const std::string& fieldName,
                           const value::Value& value);

        // Variable access
        value::Value getVariable(const std::string& variableName);
        void setVariable(const std::string& variableName, const value::Value& value);

        // Object creation and manipulation
        value::Value createObject(const std::string& className,
                                 const std::vector<value::Value>& constructorArgs = {});

        // Utility methods for working with objects
        bool isObjectOfClass(const value::Value& object, const std::string& className);
        std::string getObjectClassName(const value::Value& object);
    };
}
