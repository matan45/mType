#pragma once
#include "SlotType.hpp"
#include "JitContext.hpp"
#include "JitHelpers.hpp"
#include "LoopProfiler.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace vm::jit
{
    class JitCodeCache;
    namespace ic { class TypeFeedbackCollector; }
    enum class CmpOp { EQ, NE, LT, GT, LE, GE };

    struct JitEmissionState
    {
        asmjit::x86::Compiler& cc;

        asmjit::x86::Gp ctxPtr;
        asmjit::x86::Gp localsBase;
        asmjit::x86::Gp stackBase;
        asmjit::x86::Gp boxedBase;
        asmjit::x86::Gp progPtr;

        bool usesBoxedTypes;
        size_t localCount;
        size_t localStride;

        int stackDepth = 0;
        std::vector<SlotType> slotTypes;
        std::unordered_map<size_t, SlotType> localTypes;
        bool compileFailed = false;
        // MYT-148: when compileFailed is set during OSR emission, record which
        // gate tripped so compileLoopOSR can mark the loop profile with a
        // diagnosable reason. Ignored by function-level compilation.
        OSRBailoutReason osrBailoutReason = OSRBailoutReason::NONE;
        uint8_t osrBailoutOpcode = 0;
        size_t currentIP = 0;
        std::unordered_map<size_t, asmjit::Label> labels;

        const bytecode::BytecodeProgram& program;

        ic::TypeFeedbackCollector* typeFeedback = nullptr;

        struct CachedArrayInfo {
            asmjit::x86::Gp dataPtr;
            asmjit::x86::Gp length;
        };
        std::unordered_map<int, CachedArrayInfo> arrayInfoCache;
        std::unordered_set<size_t> backEdgeTargets;

        static constexpr size_t MAX_OP_STACK = 64;
        static constexpr size_t VALUE_SIZE = sizeof(value::Value);
    };

    void emitBox(JitEmissionState& s, asmjit::x86::Gp destAddr,
                 int stackOff, SlotType type);
    void emitUnbox(JitEmissionState& s, asmjit::x86::Gp srcAddr,
                   int stackOff, SlotType type);
    void emitEnsureUnboxed(JitEmissionState& s, int stackIdx,
                           SlotType type, SlotType targetType);
    void emitBoxOrCopy(JitEmissionState& s, asmjit::x86::Gp destAddr,
                       int stackOff, SlotType type);
    SlotType popType(JitEmissionState& s);
    SlotType topType(const JitEmissionState& s);
    void emitCleanup(JitEmissionState& s);
    void emitGenericBinop(JitEmissionState& s, uint64_t helperFn,
                          SlotType lType, SlotType rType);
    void emitCmp(JitEmissionState& s, CmpOp kind);

    bool emitCoreOps(JitEmissionState& s,
                     const bytecode::BytecodeProgram::Instruction& instr);

    bool emitArithmeticOps(JitEmissionState& s,
                           const bytecode::BytecodeProgram::Instruction& instr);

    using ExitHandler = std::function<void(JitEmissionState& s, size_t target)>;
    bool emitControlFlowOps(JitEmissionState& s,
                            const bytecode::BytecodeProgram::Instruction& instr,
                            const ExitHandler& onExit);

    bool emitArrayOps(JitEmissionState& s,
                      const bytecode::BytecodeProgram::Instruction& instr);

    bool emitObjectOps(JitEmissionState& s,
                       const bytecode::BytecodeProgram::Instruction& instr);

    void emitValueDestroy(JitEmissionState& s, int slotOffset);
    void emitReturnValueCopyBoxed(JitEmissionState& s);
    asmjit::x86::Gp emitGetBoxedValueAddr(JitEmissionState& s, int stackIdx, SlotType valType);

    void emitBoxCallArgs(JitEmissionState& s, size_t argCount,
                         size_t destStartSlot = 0);
    void emitPopAndDestroyArgs(JitEmissionState& s, size_t argCount);

    void emitLocalsWriteBack(JitEmissionState& s);

    bool scanOpcodesForBoxedTypes(const bytecode::BytecodeProgram& program,
                                  size_t startOffset, size_t endOffset);

    std::unordered_map<size_t, asmjit::Label> createJumpLabels(
        asmjit::x86::Compiler& cc, const bytecode::BytecodeProgram& program,
        size_t startOffset, size_t endOffset,
        size_t rangeStart = 0, size_t rangeEnd = SIZE_MAX);

    std::unordered_set<size_t> collectBackEdgeTargets(
        const bytecode::BytecodeProgram& program,
        size_t startOffset, size_t endOffset);

    void emitMemoryInit(asmjit::x86::Compiler& cc,
                        asmjit::x86::Gp localsBase, size_t localCount,
                        asmjit::x86::Gp boxedBase, size_t boxedCount);

    bool finalizeAndStore(asmjit::x86::Compiler& cc, asmjit::CodeHolder& code,
                          JitCodeCache& codeCache, const std::string& key,
                          size_t& compileCount, size_t& bailoutCount);
}
