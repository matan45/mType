#pragma once
#include "../context/ExecutionContext.hpp"
#include "../validation/AccessValidator.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include <vector>
#include <string>
#include <unordered_map>

namespace vm::runtime
{
    /**
     * Helper class for object instance operations
     * Extracted from ObjectExecutor to improve Single Responsibility Principle compliance
     * Handles: object creation, field initialization, super constructor/method calls
     */
    class ObjectInstanceHelper
    {
    public:
        explicit ObjectInstanceHelper(ExecutionContext& ctx);
        ~ObjectInstanceHelper() = default;

        // Object creation
        void handleNewObject(const bytecode::BytecodeProgram::Instruction& instr);

        // Super calls
        void handleSuperConstructor(const bytecode::BytecodeProgram::Instruction& instr);
        void handleThisConstructor(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperInvoke(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperGetField(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperSetField(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;

        // Object creation helpers
        std::string parseGenericTypeArguments(const std::string& fullClassName,
                                              std::unordered_map<std::string, std::string>& genericTypeBindings);
        std::vector<value::Value> prepareConstructorArguments(size_t argCount);
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> createObjectInstance(
            const std::string& baseClassName,
            const std::unordered_map<std::string, std::string>& genericTypeBindings);
        void invokeConstructor(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                              const std::string& baseClassName,
                              const std::vector<value::Value>& args);
        void initializeObjectFields(
            std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef);

        // Access control utilities
        validation::AccessContext createAccessContext(const std::string& targetClassName, bool isSetter = false);
        std::string getCurrentClassName();
        bool isSubclass(const std::string& derivedClass, const std::string& baseClass);
    };
}
