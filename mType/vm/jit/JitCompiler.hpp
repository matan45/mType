#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "JitCodeCache.hpp"
#include "JitContext.hpp"
#include "SlotType.hpp"
#include "OSRState.hpp"
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

        bool compileLoopOSR(size_t loopStartOffset,
                            size_t loopEndOffset,
                            size_t jumpBackOffset,
                            const std::vector<LocalSlotInfo>& localSlotInfos,
                            size_t localCount,
                            const bytecode::BytecodeProgram& program,
                            JitCodeCache& codeCache,
                            ic::TypeFeedbackCollector* typeFeedback = nullptr);

    private:
        bool canCompile(const bytecode::BytecodeProgram::FunctionMetadata& meta,
                        const bytecode::BytecodeProgram& program) const;

        bool canCompileLoopOSR(size_t loopStartOffset, size_t loopEndOffset,
                               const bytecode::BytecodeProgram& program) const;

        static const std::unordered_set<uint8_t>& getSupportedOpcodes();

        size_t compileCount = 0;
        size_t bailoutCount = 0;
    };
}
