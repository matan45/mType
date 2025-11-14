#pragma once
#include "../context/ExecutionContext.hpp"
#include "../validation/AccessValidator.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/NullPointerException.hpp"
#include "../../../errors/FieldNotFoundException.hpp"
#include "../../../errors/AccessViolationException.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../ast/AccessModifier.hpp"
#include <unordered_map>
#include <memory>

namespace vm::runtime
{
    // Forward declarations
    class FunctionExecutor;
    class ObjectInstanceHelper;

    /**
     * Executes object-related opcodes
     * Handles NEW_OBJECT, GET/SET_FIELD, GET/SET_STATIC, CALL_METHOD, SUPER_CONSTRUCTOR, SUPER_INVOKE
     * Manages object creation, field access, method calls, and inheritance
     */
    class ObjectExecutor
    {
    public:
        explicit ObjectExecutor(ExecutionContext& ctx);
        ~ObjectExecutor();

        // Set FunctionExecutor reference for lambda-to-interface conversion
        void setFunctionExecutor(FunctionExecutor* funcExec) { functionExecutor = funcExec; }

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
        void handleThisConstructor(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperInvoke(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperGetField(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperSetField(const bytecode::BytecodeProgram::Instruction& instr);

        // Iterator operations
        void handleGetIterator(const bytecode::BytecodeProgram::Instruction& instr);
        void handleIteratorHasNext(const bytecode::BytecodeProgram::Instruction& instr);
        void handleIteratorNext(const bytecode::BytecodeProgram::Instruction& instr);
        void handleIteratorClose(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;
        FunctionExecutor* functionExecutor = nullptr;
        std::unique_ptr<ObjectInstanceHelper> instanceHelper;

        // Helper methods for access context creation (used by field access operations)
        validation::AccessContext createAccessContext(
            const std::string& targetClassName,
            bool isSetter = false
        );

        std::string getCurrentClassName();
        bool isSubclass(const std::string& derivedClass, const std::string& baseClass);

        // Helper methods for handleCallMethod
        std::vector<value::Value> prepareMethodCallArguments(size_t argCount);
        void invokeLambdaMethod(std::shared_ptr<BytecodeLambda> lambda,
                               const std::vector<value::Value>& args,
                               const std::string& methodName);
        void invokeInstanceMethod(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                 const std::string& methodName,
                                 const std::vector<value::Value>& args,
                                 size_t argCount);
    };
}
