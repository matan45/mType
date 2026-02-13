#pragma once
#include "SlotType.hpp"
#include "JitContext.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>
#include <vector>
#include <unordered_map>
#include <functional>

namespace vm::jit
{
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
        size_t currentIP = 0;
        std::unordered_map<size_t, asmjit::Label> labels;

        const bytecode::BytecodeProgram& program;

        static constexpr size_t MAX_OP_STACK = 64;
        static constexpr size_t VALUE_SIZE = sizeof(value::Value);
    };

    // Shared emission helpers
    void emitBox(JitEmissionState& s, asmjit::x86::Gp destAddr,
                 int stackOff, SlotType type);
    void emitUnbox(JitEmissionState& s, asmjit::x86::Gp srcAddr,
                   int stackOff, SlotType type);
    SlotType popType(JitEmissionState& s);
    SlotType topType(const JitEmissionState& s);
    void emitCleanup(JitEmissionState& s);
    void emitGenericBinop(JitEmissionState& s, uint64_t helperFn,
                          SlotType lType, SlotType rType);
    void emitCmp(JitEmissionState& s, CmpOp kind);

    // Category emitters — each returns true if the opcode was handled
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

    // Shared helpers for call-type opcodes
    void emitBoxCallArgs(JitEmissionState& s, size_t argCount,
                         size_t destStartSlot = 0);
    void emitPopAndDestroyArgs(JitEmissionState& s, size_t argCount);

    // OSR-specific
    void emitLocalsWriteBack(JitEmissionState& s);
}
