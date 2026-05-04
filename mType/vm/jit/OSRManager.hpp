#pragma once
#include <memory>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include "LoopProfiler.hpp"
#include "OSRState.hpp"
#include "JitCodeCache.hpp"
#include "JitCompiler.hpp"
#include "JitContext.hpp"
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

        // Analyze loop structure and build OSR state from current interpreter
        // state. Returns OSRBailoutReason::NONE on success, or the specific
        // reason (LOOP_MARKERS_MISSING / SHARED_FRAME_REJECTION / ...) on
        // failure so the caller can record it in the loop profile (MYT-148).
        OSRBailoutReason captureState(OSRState& state,
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

        // Find LOOP_START and LOOP_END boundaries around a jump-back offset
        bool findLoopBoundaries(size_t jumpBackOffset,
                               const bytecode::BytecodeProgram& program,
                               size_t& loopStartOffset,
                               size_t& loopEndOffset);

        // Capture local variables from the interpreter stack into OSRState
        void captureLocals(OSRState& state,
                          size_t localBase,
                          size_t localCount,
                          vm::runtime::ExecutionContext& context);

        // Build JitContext for OSR loop execution
        void buildOSRContext(JitContext& jitCtx,
                            const OSRState& state,
                            const bytecode::BytecodeProgram& program,
                            vm::runtime::ExecutionContext& context,
                            vm::runtime::VirtualMachine& vm,
                            JitCodeCache& codeCache,
                            value::Value* inputLocals,
                            value::Value* outputLocals);

        // Compile a loop via JIT and cache the result. The out-parameters
        // (MYT-148) receive the specific bailout reason / offending opcode
        // when compilation fails, so the caller can tag the LoopProfile.
        bool compileAndCacheLoop(OSRState& state,
                                size_t jumpBackOffset,
                                const bytecode::BytecodeProgram& program,
                                JitCompiler& compiler,
                                JitCodeCache& codeCache,
                                ic::TypeFeedbackCollector* typeFeedback,
                                OSRBailoutReason& outReason,
                                uint8_t& outOffendingOpcode);

        // Infer SlotType from a runtime Value
        static SlotType inferSlotType(const value::Value& val);
    };
}
