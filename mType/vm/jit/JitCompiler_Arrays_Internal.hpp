#pragma once

#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include <asmjit/x86.h>
#include <cstddef>
#include <cstdint>

namespace vm::jit::detail
{
    // Extract array info (dataPtr + length) for a given boxed stack slot.
    // Uses the cache if available, otherwise calls jit_array_extract_info once.
    // Inline so both JitCompiler_Arrays.cpp (length op) and
    // JitCompiler_Arrays_Bounds.cpp (typed ops) compile their own copy without
    // crossing a TU boundary or losing inlining inside hot bounds-check loops.
    inline void emitExtractArrayInfo(JitEmissionState& s, int cacheKey,
                                     asmjit::x86::Gp arrAddr,
                                     asmjit::x86::Gp& dataPtr,
                                     asmjit::x86::Gp& arrLen)
    {
        auto& cc = s.cc;
        if (cacheKey >= 0)
        {
            auto cacheIt = s.arrayInfoCache.find(cacheKey);
            if (cacheIt != s.arrayInfoCache.end())
            {
                dataPtr = cacheIt->second.dataPtr;
                arrLen = cacheIt->second.length;
                return;
            }
        }

        asmjit::x86::Mem infoMem = cc.new_stack(16, 8);
        asmjit::x86::Gp infoAddr = cc.new_gp64();
        cc.lea(infoAddr, infoMem);

        asmjit::InvokeNode* extractInv;
        cc.invoke(asmjit::Out(extractInv),
                  reinterpret_cast<uint64_t>(jit_array_extract_info),
                  asmjit::FuncSignature::build<void, const value::Value*, JitArrayInfo*>());
        extractInv->set_arg(0, arrAddr);
        extractInv->set_arg(1, infoAddr);

        dataPtr = cc.new_gp64();
        cc.mov(dataPtr, asmjit::x86::Mem(infoAddr, 0));   // JitArrayInfo::data
        arrLen = cc.new_gp64();
        cc.mov(arrLen, asmjit::x86::Mem(infoAddr, 8));    // JitArrayInfo::length

        if (cacheKey >= 0)
            s.arrayInfoCache[cacheKey] = {dataPtr, arrLen};
    }

    // emitArrayOps in JitCompiler_Arrays.cpp dispatches typed ops via this
    // declaration, defined in JitCompiler_Arrays_Bounds.cpp.
    bool emitTypedArrayOps(JitEmissionState& s,
                           const bytecode::BytecodeProgram::Instruction& instr);
}
