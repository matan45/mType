#pragma once
#include <string>
#include <cstddef>
#include <vector>
#include "../../bytecode/BytecodeProgram.hpp"

namespace vm::runtime { class ExecutionContext; class VirtualMachine; }

namespace vm::runtime::utils
{
    /**
     * Result of resolving a method's bytecode across the main program
     * and any loaded library programs.
     *
     * program / programIndex identify which BytecodeProgram owns the
     * returned funcMetadata — callers that push a CallFrame for the
     * call must carry both so the frame restores context.program on
     * return (see ControlFlowExecutor return-handling).
     */
    struct MethodResolution
    {
        const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata = nullptr;
        const bytecode::BytecodeProgram* program = nullptr;
        size_t programIndex = 0;
        // Qualified name actually found — may differ from the input when
        // the prefix fallback matched a different overload (e.g. generic
        // type erasure produces String::equals/object but the compiled
        // function is String::equals/String).
        std::string qualifiedName;
    };

    /**
     * Resolve a method to its bytecode metadata across the caller's program
     * and all loaded library programs.
     *
     * Search order matches ObjectExecutor::invokeInstanceMethod historical
     * behaviour:
     *   1. exact qualifiedName in context.program
     *   2. prefix "definingClassName::simpleMethodName" fallback in context.program
     *   3. exact qualifiedName in each loadedPrograms[i]
     *   4. prefix fallback in each loadedPrograms[i]
     *
     * On success the result carries the owning program (pointer + index).
     * On failure funcMetadata is nullptr.
     */
    class MethodResolver
    {
    public:
        // ExecutionContext-based overload used by interpreter paths. Preserves
        // the caller's current frame programIndex as the default when the
        // method is found in context.program. `vm` supplies the loaded-program
        // catalog for library dispatch.
        static MethodResolution resolve(
            const std::string& qualifiedName,
            const std::string& definingClassName,
            const std::string& simpleMethodName,
            const ExecutionContext& context,
            const VirtualMachine& vm);

        // Raw-pointer overload used by JIT runtime helpers that only hold
        // JitContext + the VM's loadedPrograms vector. currentProgramIndex
        // seeds the default result when the method is found in currentProgram.
        static MethodResolution resolve(
            const std::string& qualifiedName,
            const std::string& definingClassName,
            const std::string& simpleMethodName,
            const bytecode::BytecodeProgram* currentProgram,
            const std::vector<const bytecode::BytecodeProgram*>* loadedPrograms,
            size_t currentProgramIndex);
    };
}
