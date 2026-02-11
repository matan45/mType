#pragma once
#include "../JitCompiler.hpp"
#include "../OSRState.hpp"
#include "../JitContext.hpp"
#include "../JitHelpers.hpp"
#include <asmjit/x86.h>
#include <unordered_map>
#include <vector>

namespace vm::jit::codegen
{
    class OSREntryCodegen
    {
    public:
        struct EntryInfo
        {
            asmjit::x86::Gp localsBase;
            asmjit::x86::Gp stackBase;
            asmjit::x86::Gp boxedBase;
            asmjit::x86::Gp ctxPtr;
            asmjit::x86::Gp progPtr;
            size_t localCount;
            size_t localStride;
            bool usesBoxedTypes;
        };

        // Emit code to load all locals from the OSR state buffer in JitContext
        // into the JIT's locals area. Called once at the start of the OSR function.
        static void emitStateLoad(asmjit::x86::Compiler& cc,
                                  const EntryInfo& info,
                                  const std::vector<LocalSlotInfo>& localSlotInfos,
                                  std::unordered_map<size_t, SlotType>& localTypes);
    };
}
