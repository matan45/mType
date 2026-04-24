#pragma once
#include "../context/ExecutionContext.hpp"
#include "../validation/AccessValidator.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/NullPointerException.hpp"
#include "../../../errors/FieldNotFoundException.hpp"
#include "../../../errors/AccessViolationException.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../ast/AccessModifier.hpp"
#include <span>
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
        // MYT-134: non-escaping allocation promoted by escape-analysis — pool-borrowed raw pointer
        // tracked in the current call frame's stackObjects list.
        void handleNewStack(const bytecode::BytecodeProgram::Instruction& instr);
        void handleNewValueObject(const bytecode::BytecodeProgram::Instruction& instr);
        void handleObjectToValue(const bytecode::BytecodeProgram::Instruction& instr);

        // Field access
        void handleGetField(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSetField(const bytecode::BytecodeProgram::Instruction& instr);
        void handleInlineSetField(const bytecode::BytecodeProgram::Instruction& instr);
        void handleInlineGetField(const bytecode::BytecodeProgram::Instruction& instr);
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

        // Access context helpers (also used by InlineCacheExecutor)
        validation::AccessContext createAccessContext(
            const std::string& targetClassName,
            bool isSetter = false
        );
        std::string getCurrentClassName();
        bool isSubclass(const std::string& derivedClass, const std::string& baseClass);

    private:
        ExecutionContext& context;
        FunctionExecutor* functionExecutor = nullptr;
        std::unique_ptr<ObjectInstanceHelper> instanceHelper;

        // Helper methods for handleCallMethod.
        // MYT-196: args passed as std::span so callers can back them with a
        // SmallArgsBuffer (no per-call heap alloc) or any vector-like.
        void invokeLambdaMethod(std::shared_ptr<BytecodeLambda> lambda,
                               std::span<const value::Value> args,
                               const std::string& methodName);
        void invokeInstanceMethod(std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                 const std::string& methodName,
                                 std::span<const value::Value> args,
                                 size_t argCount);

        // Value-class receiver dispatch — batch-materialises a temp
        // ObjectInstance from the ValueObject's field vector (via
        // ObjectInstance::loadFromValueObject) and delegates to the regular
        // instance path. Preserves in-method mutation semantics while
        // avoiding the per-field setField hashmap thrash of the previous
        // per-call hot loop.
        void invokeValueObjectMethod(std::shared_ptr<value::ValueObject> valueObj,
                                     const std::string& methodName,
                                     std::span<const value::Value> args,
                                     size_t argCount);
    };
}
