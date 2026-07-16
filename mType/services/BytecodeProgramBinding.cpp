#include "BytecodeProgramBinding.hpp"

#include "ScriptAPI.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"
#include "../vm/runtime/VirtualMachine.hpp"

#include <stdexcept>
#include <utility>

namespace services
{
    void BytecodeProgramBinding::clearCurrent(
        const std::shared_ptr<vm::runtime::VirtualMachine>& vm,
        ScriptAPI* scriptAPI)
    {
        const bool hasVmBinding = vm && vm->getProgram() != nullptr;
        const bool hasApiBinding = scriptAPI
            && scriptAPI->getBytecodeProgram() != nullptr;
        if (!hasVmBinding && !hasApiBinding) return;

        if (vm)
        {
            // Invalidate tasks and promise continuations before clearing the
            // frames/caches they can resume into. The currently bound owner is
            // deliberately still alive for this entire operation.
            vm->prepareForProgramReplacement();
            vm->reset();
            vm->setProgram(nullptr);
        }
        if (scriptAPI)
        {
            scriptAPI->setBytecodeProgram(nullptr);
        }
    }

    void BytecodeProgramBinding::bind(
        const std::shared_ptr<vm::runtime::VirtualMachine>& vm,
        ScriptAPI* scriptAPI,
        const vm::bytecode::BytecodeProgram& program)
    {
        if (vm)
        {
            vm->setProgram(&program);
        }
        if (scriptAPI)
        {
            scriptAPI->setBytecodeProgram(&program);
        }
    }

    void BytecodeProgramBinding::replace(
        const std::shared_ptr<vm::runtime::VirtualMachine>& vm,
        ScriptAPI* scriptAPI,
        ProgramOwner& owner,
        ProgramOwner replacement)
    {
        if (!replacement)
        {
            throw std::invalid_argument(
                "cannot bind an empty BytecodeProgram owner");
        }

        // The replacement is fully constructed before this call. If cleanup
        // throws, both owners remain alive and the caller can report failure
        // without exposing a dangling binding.
        clearCurrent(vm, scriptAPI);
        owner.reset();
        owner = std::move(replacement);
        bind(vm, scriptAPI, *owner);
    }

    void BytecodeProgramBinding::release(
        const std::shared_ptr<vm::runtime::VirtualMachine>& vm,
        ScriptAPI* scriptAPI,
        ProgramOwner& owner)
    {
        if (!owner) return;

        const auto* retiring = owner.get();
        const bool vmBound = vm && vm->getProgram() == retiring;
        const bool apiBound = scriptAPI
            && scriptAPI->getBytecodeProgram() == retiring;

        // An inactive owner was already invalidated when another program was
        // bound. Do not tear down that newer program merely because this older
        // owner is now leaving scope.
        if (vmBound || apiBound)
        {
            clearCurrent(vm, scriptAPI);
        }
        owner.reset();
    }
}
