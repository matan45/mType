#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/NullPointerException.hpp"
#include "../../../errors/FieldNotFoundException.hpp"
#include "../../../errors/AccessViolationException.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../ast/AccessModifier.hpp"
#include <unordered_map>

namespace vm::runtime
{
    /**
     * Executes object-related opcodes
     * Handles NEW_OBJECT, GET/SET_FIELD, GET/SET_STATIC, CALL_METHOD, SUPER_CONSTRUCTOR, SUPER_INVOKE
     * Manages object creation, field access, method calls, and inheritance
     */
    class ObjectExecutor
    {
    public:
        explicit ObjectExecutor(ExecutionContext& ctx);
        ~ObjectExecutor() = default;

        // Object creation
        void handleNewObject(const bytecode::BytecodeProgram::Instruction& instr);

        // Field access
        void handleGetField(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSetField(const bytecode::BytecodeProgram::Instruction& instr);
        void handleGetStatic(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSetStatic(const bytecode::BytecodeProgram::Instruction& instr);

        // Method calls
        void handleCallMethod(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperConstructor(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperInvoke(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;

        // Helper methods
        void initializeObjectFields(
            std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef
        );

        void validateConstructorAccess(
            const std::string& className,
            ast::AccessModifier accessMod
        );

        void validateMethodAccess(
            const std::string& className,
            const std::string& methodName,
            ast::AccessModifier accessMod
        );

        void validateFieldAccess(
            const std::string& className,
            const std::string& fieldName,
            ast::AccessModifier accessMod
        );

        std::string getCurrentClassName();
        bool isSubclass(const std::string& derivedClass, const std::string& baseClass);
    };
}
