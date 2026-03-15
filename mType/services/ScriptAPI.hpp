#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"

namespace vm::runtime
{
    class VirtualMachine;
}

namespace vm::bytecode
{
    class BytecodeProgram;
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

        // Bytecode VM support
        vm::runtime::VirtualMachine* vm;
        const vm::bytecode::BytecodeProgram* program;

    public:
        ScriptAPI(std::shared_ptr<environment::Environment> env,
                 vm::runtime::VirtualMachine* virtualMachine,
                 const vm::bytecode::BytecodeProgram* bytecodeProgram = nullptr);
        ~ScriptAPI();

        // Update bytecode program reference (for bytecode mode)
        void setBytecodeProgram(const vm::bytecode::BytecodeProgram* bytecodeProgram);

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

        // Lambda invocation from C++
        value::Value callLambda(const value::Value& lambda,
                               const std::vector<value::Value>& args = {});

        // Instance field access
        value::Value getField(const value::Value& object, const std::string& fieldName);
        void setField(const value::Value& object,
                     const std::string& fieldName,
                     const value::Value& value);

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
