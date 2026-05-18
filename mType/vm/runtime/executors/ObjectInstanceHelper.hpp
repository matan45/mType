#pragma once
#include "../context/ExecutionContext.hpp"
#include "../../../environment/Environment.hpp"
#include "../validation/AccessValidator.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include <span>
#include <vector>
#include <string>
#include <unordered_map>

namespace vm::runtime
{
    class VirtualMachine;

    /**
     * Helper class for object instance operations
     * Extracted from ObjectExecutor to improve Single Responsibility Principle compliance
     * Handles: object creation, field initialization, super constructor/method calls
     */
    class ObjectInstanceHelper
    {
    public:
        ObjectInstanceHelper(ExecutionContext& ctx,
                             std::shared_ptr<environment::Environment> env,
                             VirtualMachine* vmPtr);
        ~ObjectInstanceHelper() = default;

        // Object creation
        void handleNewObject(const bytecode::BytecodeProgram::Instruction& instr);
        // MYT-134: escape-analysis-promoted allocation. Uses ObjectInstancePool::acquireRaw
        // to skip shared_ptr wrapping's owning control block on the hot path AND skips
        // GC registration; lifetime is tied to the current CallFrame's stackObjects list,
        // which releases the pointer back to the pool at frame teardown. Constructor
        // invocation uses an aliasing shared_ptr with a no-op deleter to keep the
        // existing invokeConstructor machinery unchanged in this iteration — a follow-up
        // ticket extends the field/method executors to accept STACK_OBJECT values
        // directly and eliminate the aliasing shared_ptr entirely.
        void handleNewStack(const bytecode::BytecodeProgram::Instruction& instr);

        // Super calls
        void handleSuperConstructor(const bytecode::BytecodeProgram::Instruction& instr);
        void handleThisConstructor(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperInvoke(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperGetField(const bytecode::BytecodeProgram::Instruction& instr);
        void handleSuperSetField(const bytecode::BytecodeProgram::Instruction& instr);

    private:
        ExecutionContext& context;
        std::shared_ptr<environment::Environment> environment;
        VirtualMachine* vm;

        // Object creation helpers
        std::string parseGenericTypeArguments(const std::string& fullClassName,
                                              std::unordered_map<std::string, std::string>& genericTypeBindings);
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> createObjectInstance(
            const std::string& baseClassName,
            const std::unordered_map<std::string, std::string>& genericTypeBindings);
        // MYT-196: args passed as span so callers can back them with a
        // SmallArgsBuffer and skip the per-call heap allocation.
        // MYT-208: receiver passed as Value so OBJECT (heap, shared_ptr via
        // bridge) and STACK_OBJECT (raw borrowed) flow through the same
        // dispatch. Tag-branches the new ctor frame's thisInstance vs
        // thisInstanceRaw assignment and pushes the receiver Value onto the
        // operand stack preserving the tag.
        void invokeConstructor(const value::Value& receiverValue,
                              const std::string& baseClassName,
                              std::span<const value::Value> args);
        // MYT-208: takes a raw ObjectInstance* — works for shared_ptr-owned
        // (call .get()) and borrowed STACK_OBJECT pointers without re-wrap.
        void initializeObjectFields(
            runtimeTypes::klass::ObjectInstance* instance,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef);

        // Access control utilities
        validation::AccessContext createAccessContext(const std::string& targetClassName, bool isSetter = false);
        std::string getCurrentClassName();
        bool isSubclass(const std::string& derivedClass, const std::string& baseClass);
    };
}
