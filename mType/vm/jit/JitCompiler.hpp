#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "JitCodeCache.hpp"
#include "JitContext.hpp"
#include "SlotType.hpp"
#include "OSRState.hpp"
#include "LoopProfiler.hpp"
namespace vm::jit::ic { class TypeFeedbackCollector; }
#include "../bytecode/BytecodeProgram.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    class JitCompiler
    {
    public:
        JitCompiler();

        bool compile(const std::string& functionName,
                     const bytecode::BytecodeProgram& program,
                     JitCodeCache& codeCache,
                     ic::TypeFeedbackCollector* typeFeedback = nullptr);

        size_t getCompileCount() const { return compileCount; }
        size_t getBailoutCount() const { return bailoutCount; }

        // MYT-172 AC #3: inline field-IC hit/miss counters bumped from JIT
        // emitted code. Pointers are baked as immediates at emit time, so
        // these members must live as long as any compiled function — which
        // they do, since JitCompiler is owned by the VirtualMachine.
        uint64_t getInlineFieldICHits() const { return inlineFieldICHits; }
        uint64_t getInlineFieldICMisses() const { return inlineFieldICMisses; }
        uint64_t* inlineFieldICHitsPtr() { return &inlineFieldICHits; }
        uint64_t* inlineFieldICMissesPtr() { return &inlineFieldICMisses; }

        // MYT-148: extra out-parameters so the caller (OSRManager) can record
        // WHICH gate rejected the loop in the LoopProfile for --jit-stats.
        // Defaults preserve pre-MYT-148 behavior for any other call site.
        bool compileLoopOSR(size_t loopStartOffset,
                            size_t loopEndOffset,
                            size_t jumpBackOffset,
                            const std::vector<LocalSlotInfo>& localSlotInfos,
                            size_t localCount,
                            const bytecode::BytecodeProgram& program,
                            JitCodeCache& codeCache,
                            ic::TypeFeedbackCollector* typeFeedback = nullptr,
                            OSRBailoutReason* outReason = nullptr,
                            uint8_t* outOffendingOpcode = nullptr);

    private:
        bool canCompile(const bytecode::BytecodeProgram::FunctionMetadata& meta,
                        const bytecode::BytecodeProgram& program) const;

        // Returns true if every opcode in [start, end] is JIT-supported. On
        // failure, *outOpcode (if non-null) gets the offending opcode byte.
        bool canCompileLoopOSR(size_t loopStartOffset, size_t loopEndOffset,
                               const bytecode::BytecodeProgram& program,
                               uint8_t* outOpcode = nullptr) const;

        static const std::unordered_set<uint8_t>& getSupportedOpcodes();

        size_t compileCount = 0;
        size_t bailoutCount = 0;
        uint64_t inlineFieldICHits = 0;
        uint64_t inlineFieldICMisses = 0;
    };
}
