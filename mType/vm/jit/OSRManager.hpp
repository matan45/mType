#pragma once
#include <memory>
#include <unordered_map>
#include "LoopProfiler.hpp"
#include "OSRState.hpp"
#include "JitCodeCache.hpp"
#include "JitCompiler.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../runtime/context/ExecutionContext.hpp"

namespace vm::runtime { class VirtualMachine; }

namespace vm::jit
{
    using OSRLoopFunction = JitFunction;

    class OSRManager
    {
    public:
        OSRManager();
        ~OSRManager();

        // Called from ControlFlowExecutor::handleJumpBack() on every back-edge.
        // Returns true if OSR transition occurred and the interpreter should resume
        // at the updated instructionPointer (set by writeBackState).
        bool tryOSR(size_t jumpBackOffset,
                    const bytecode::BytecodeProgram& program,
                    vm::runtime::ExecutionContext& context,
                    vm::runtime::VirtualMachine& vm,
                    JitCompiler& compiler,
                    JitCodeCache& codeCache);

        const OSRResult& getLastResult() const { return lastResult; }

        LoopProfiler& getLoopProfiler() { return loopProfiler; }
        const LoopProfiler& getLoopProfiler() const { return loopProfiler; }

    private:
        LoopProfiler loopProfiler;
        OSRResult lastResult;

        // Cache of compiled OSR loops: jumpBackOffset -> compiled code
        std::unordered_map<size_t, OSRLoopFunction> osrCache;

        // Analyze loop structure and build OSR state from current interpreter state
        bool captureState(OSRState& state,
                         size_t jumpBackOffset,
                         const bytecode::BytecodeProgram& program,
                         vm::runtime::ExecutionContext& context);

        // Execute the OSR-compiled loop
        bool executeOSRLoop(OSRLoopFunction func,
                           const OSRState& state,
                           const bytecode::BytecodeProgram& program,
                           vm::runtime::ExecutionContext& context,
                           vm::runtime::VirtualMachine& vm,
                           JitCodeCache& codeCache);

        // Write updated locals back to interpreter stack
        void writeBackState(const OSRResult& result,
                           const OSRState& state,
                           vm::runtime::ExecutionContext& context);

        // Infer SlotType from a runtime Value
        static SlotType inferSlotType(const value::Value& val);
    };
}
