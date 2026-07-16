#pragma once

#include <memory>

namespace vm::bytecode
{
    class BytecodeProgram;
}

namespace vm::runtime
{
    class VirtualMachine;
}

namespace services
{
    class ScriptAPI;

    // Coordinates the raw BytecodeProgram pointers retained by the VM and
    // ScriptAPI with the unique_ptr that actually owns the program. Every
    // replacement must pass through this boundary so runtime caches and async
    // continuations are invalidated before the old owner releases storage.
    class BytecodeProgramBinding final
    {
    public:
        using ProgramOwner =
            std::unique_ptr<vm::bytecode::BytecodeProgram>;

        static void replace(
            const std::shared_ptr<vm::runtime::VirtualMachine>& vm,
            ScriptAPI* scriptAPI,
            ProgramOwner& owner,
            ProgramOwner replacement);

        static void release(
            const std::shared_ptr<vm::runtime::VirtualMachine>& vm,
            ScriptAPI* scriptAPI,
            ProgramOwner& owner);

        // Invalidate and remove the current raw binding without destroying an
        // owner. Used by rebuild/teardown boundaries that may also have
        // separately-owned loaded library programs.
        static void clearCurrent(
            const std::shared_ptr<vm::runtime::VirtualMachine>& vm,
            ScriptAPI* scriptAPI);

    private:
        static void bind(
            const std::shared_ptr<vm::runtime::VirtualMachine>& vm,
            ScriptAPI* scriptAPI,
            const vm::bytecode::BytecodeProgram& program);
    };
}
