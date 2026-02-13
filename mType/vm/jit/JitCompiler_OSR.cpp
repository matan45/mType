#include "JitCompiler.hpp"
#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "codegen/OSREntryCodegen.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>
#include <unordered_set>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    // ===== OSR locals write-back =====

    void emitLocalsWriteBack(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        Gp outPtr = cc.new_gp64();
        cc.mov(outPtr, Mem(s.ctxPtr, offsetof(JitContext, osrOutputLocals)));

        for (size_t i = 0; i < s.localCount; ++i)
        {
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(outPtr, static_cast<int32_t>(i * valueSize)));

            SlotType lt = s.localTypes.count(i) ? s.localTypes[i] : SlotType::INT;

            if (s.usesBoxedTypes)
            {
                Gp srcAddr = cc.new_gp64();
                cc.lea(srcAddr, Mem(s.localsBase, static_cast<int32_t>(i * s.localStride)));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                          FuncSignature::build<void, value::Value*, const value::Value*>());
                inv->set_arg(0, destAddr);
                inv->set_arg(1, srcAddr);
            }
            else
            {
                if (lt == SlotType::FLOAT)
                {
                    Vec val = cc.new_xmm();
                    cc.movss(val, Mem(s.localsBase, static_cast<int32_t>(i * 8)));
                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_box_float),
                              FuncSignature::build<void, value::Value*, float>());
                    inv->set_arg(0, destAddr);
                    inv->set_arg(1, val);
                }
                else if (lt == SlotType::BOOL)
                {
                    Gp val = cc.new_gp64();
                    cc.mov(val, Mem(s.localsBase, static_cast<int32_t>(i * 8)));
                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_box_bool),
                              FuncSignature::build<void, value::Value*, int64_t>());
                    inv->set_arg(0, destAddr);
                    inv->set_arg(1, val);
                }
                else
                {
                    Gp val = cc.new_gp64();
                    cc.mov(val, Mem(s.localsBase, static_cast<int32_t>(i * 8)));
                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_box_int),
                              FuncSignature::build<void, value::Value*, int64_t>());
                    inv->set_arg(0, destAddr);
                    inv->set_arg(1, val);
                }
            }
        }
    }

    // ===== compileLoopOSR() =====

    bool JitCompiler::compileLoopOSR(size_t loopStartOffset,
                                      size_t loopEndOffset,
                                      size_t jumpBackOffset,
                                      const std::vector<LocalSlotInfo>& localSlotInfos,
                                      size_t localCount,
                                      const bytecode::BytecodeProgram& program,
                                      JitCodeCache& codeCache)
    {
        std::string osrKey = "osr@" + std::to_string(jumpBackOffset);
        if (codeCache.contains(osrKey))
            return true;

        if (!canCompileLoopOSR(loopStartOffset, loopEndOffset, program))
        {
            bailoutCount++;
            return false;
        }

        if (localCount == 0) localCount = 1;
        constexpr size_t MAX_LOCAL_COUNT = 1024;
        if (localCount > MAX_LOCAL_COUNT)
        {
            bailoutCount++;
            return false;
        }
        constexpr size_t MAX_OP_STACK = 64;
        constexpr size_t valueSize = sizeof(value::Value);

        // Pre-scan loop body for boxed type usage
        bool usesBoxedTypes = false;
        {
            for (size_t ip = loopStartOffset; ip <= loopEndOffset; ++ip)
            {
                const auto& si = program.getInstruction(ip);
                switch (si.opcode)
                {
                    case OpCode::PUSH_STRING: case OpCode::GET_FIELD:
                    case OpCode::SET_FIELD:   case OpCode::NEW_OBJECT:
                    case OpCode::CALL_METHOD: case OpCode::CALL_STATIC:
                    case OpCode::NEW_ARRAY:   case OpCode::ARRAY_GET:
                    case OpCode::ARRAY_SET:   case OpCode::ARRAY_LENGTH:
                    case OpCode::INSTANCEOF:  case OpCode::CAST:
                        usesBoxedTypes = true;
                        break;
                    default: break;
                }
                if (usesBoxedTypes) break;
            }
            if (!usesBoxedTypes)
            {
                for (size_t ip = loopStartOffset; ip <= loopEndOffset; ++ip)
                {
                    const auto& si = program.getInstruction(ip);
                    if (si.opcode == OpCode::CALL && si.operands.size() >= 2)
                    {
                        uint32_t fnIdx = static_cast<uint32_t>(si.operands[0]);
                        if (fnIdx >= program.getConstantPool().strings.size())
                            continue;
                        const std::string& fn = program.getConstantPool().getString(fnIdx);
                        const auto* cm = program.getFunction(fn);
                        if (cm && cm->returnType != "int" && cm->returnType != "float" &&
                            cm->returnType != "bool" && cm->returnType != "void")
                        {
                            usesBoxedTypes = true;
                            break;
                        }
                    }
                }
            }
            if (!usesBoxedTypes)
            {
                for (const auto& ls : localSlotInfos)
                {
                    if (isBoxedSlotType(ls.type))
                    {
                        usesBoxedTypes = true;
                        break;
                    }
                }
            }
        }

        const size_t localStride = usesBoxedTypes ? valueSize : 8;

        CodeHolder code;
        code.init(codeCache.getRuntime().environment());
        Compiler cc(&code);

        FuncNode* func = cc.add_func(FuncSignature::build<void, JitContext*>());
        Gp ctxPtr = cc.new_gp64("ctx");
        func->set_arg(0, ctxPtr);

        Mem localsArea = cc.new_stack(static_cast<uint32_t>(localCount * localStride), 8);
        Gp localsBase = cc.new_gp64("localsBase");
        cc.lea(localsBase, localsArea);

        Mem stackArea = cc.new_stack(MAX_OP_STACK * 8, 8);
        Gp stackBase = cc.new_gp64("stackBase");
        cc.lea(stackBase, stackArea);

        Gp boxedBase = cc.new_gp64("boxedBase");
        if (usesBoxedTypes)
        {
            Mem boxedArea = cc.new_stack(static_cast<uint32_t>(MAX_OP_STACK * valueSize), 8);
            cc.lea(boxedBase, boxedArea);
        }

        Gp progPtr = cc.new_gp64("progPtr");
        if (usesBoxedTypes)
        {
            cc.mov(progPtr, reinterpret_cast<uint64_t>(&program));
        }

        std::unordered_map<size_t, SlotType> localTypes;

        // Initialize locals and boxed stacks
        if (usesBoxedTypes)
        {
            Gp initBase = cc.new_gp64();
            cc.mov(initBase, localsBase);
            Gp initCount = cc.new_gp64();
            cc.mov(initCount, static_cast<int64_t>(localCount));
            InvokeNode* initLocals;
            cc.invoke(Out(initLocals), reinterpret_cast<uint64_t>(jit_locals_init),
                      FuncSignature::build<void, value::Value*, size_t>());
            initLocals->set_arg(0, initBase);
            initLocals->set_arg(1, initCount);

            Gp bsBase = cc.new_gp64();
            cc.mov(bsBase, boxedBase);
            Gp bsCount = cc.new_gp64();
            cc.mov(bsCount, static_cast<int64_t>(MAX_OP_STACK));
            InvokeNode* initBoxed;
            cc.invoke(Out(initBoxed), reinterpret_cast<uint64_t>(jit_locals_init),
                      FuncSignature::build<void, value::Value*, size_t>());
            initBoxed->set_arg(0, bsBase);
            initBoxed->set_arg(1, bsCount);
        }

        // OSR Prologue: load captured locals from JitContext.osrLocals
        codegen::OSREntryCodegen::EntryInfo entryInfo;
        entryInfo.localsBase = localsBase;
        entryInfo.stackBase = stackBase;
        entryInfo.boxedBase = boxedBase;
        entryInfo.ctxPtr = ctxPtr;
        entryInfo.progPtr = progPtr;
        entryInfo.localCount = localCount;
        entryInfo.localStride = localStride;
        entryInfo.usesBoxedTypes = usesBoxedTypes;
        codegen::OSREntryCodegen::emitStateLoad(cc, entryInfo, localSlotInfos, localTypes);

        // Zero-init non-captured locals in non-boxed mode
        if (!usesBoxedTypes)
        {
            std::unordered_set<size_t> capturedSlots;
            for (const auto& ls : localSlotInfos)
                capturedSlots.insert(ls.slot);
            for (size_t i = 0; i < localCount; ++i)
            {
                if (capturedSlots.find(i) == capturedSlots.end())
                    cc.mov(Mem(localsBase, static_cast<int32_t>(i * 8)), 0);
            }
        }

        // Create jump target labels for the loop body range
        std::unordered_map<size_t, Label> labels;
        for (size_t ip = loopStartOffset; ip <= loopEndOffset; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (instr.opcode == OpCode::JUMP || instr.opcode == OpCode::JUMP_IF_FALSE ||
                instr.opcode == OpCode::JUMP_IF_TRUE || instr.opcode == OpCode::JUMP_BACK)
            {
                if (!instr.operands.empty())
                {
                    size_t target = instr.operands[0];
                    if (target >= loopStartOffset && target <= loopEndOffset)
                    {
                        if (labels.find(target) == labels.end())
                            labels[target] = cc.new_label();
                    }
                }
            }
        }

        size_t resumeOffset = loopEndOffset + 1;

        // Build emission state
        JitEmissionState s{cc, ctxPtr, localsBase, stackBase, boxedBase, progPtr,
                           usesBoxedTypes, localCount, localStride,
                           0, {}, localTypes, false, labels, program};

        // OSR exit handler: write back locals and return
        ExitHandler osrExit = [&](JitEmissionState& es, size_t target) {
            emitLocalsWriteBack(es);
            size_t exitTarget = (target == 0) ? resumeOffset : target;
            es.cc.mov(Mem(es.ctxPtr, offsetof(JitContext, osrExitOffset)),
                      static_cast<int64_t>(exitTarget));
            es.cc.mov(Mem(es.ctxPtr, offsetof(JitContext, osrExited)), 1);
            emitCleanup(es);
            es.cc.ret();
        };

        // Phase 4 opcodes that bail out in OSR (only PUSH_STRING and GET_FIELD supported)
        static const std::unordered_set<uint8_t> osrBailoutOpcodes = {
            static_cast<uint8_t>(OpCode::SET_FIELD),
            static_cast<uint8_t>(OpCode::NEW_ARRAY),
            static_cast<uint8_t>(OpCode::ARRAY_GET),
            static_cast<uint8_t>(OpCode::ARRAY_SET),
            static_cast<uint8_t>(OpCode::ARRAY_LENGTH),
            static_cast<uint8_t>(OpCode::CALL_STATIC),
            static_cast<uint8_t>(OpCode::CALL_METHOD),
            static_cast<uint8_t>(OpCode::INSTANCEOF),
            static_cast<uint8_t>(OpCode::CAST),
            static_cast<uint8_t>(OpCode::NEW_OBJECT),
        };

        // Main codegen loop
        for (size_t ip = loopStartOffset; ip <= loopEndOffset && !s.compileFailed; ++ip)
        {
            auto labelIt = s.labels.find(ip);
            if (labelIt != s.labels.end())
                cc.bind(labelIt->second);

            const auto& instr = program.getInstruction(ip);

            // Check for unsupported Phase 4 opcodes in OSR
            if (osrBailoutOpcodes.count(static_cast<uint8_t>(instr.opcode)))
            {
                s.compileFailed = true;
                continue;
            }

            if (emitCoreOps(s, instr)) continue;
            if (emitArithmeticOps(s, instr)) continue;
            if (emitControlFlowOps(s, instr, osrExit)) continue;
            emitObjectOps(s, instr);
        }

        if (s.compileFailed)
        {
            bailoutCount++;
            return false;
        }

        // Fallthrough cleanup
        emitCleanup(s);
        cc.mov(Mem(ctxPtr, offsetof(JitContext, hasReturnValue)), 0);
        cc.end_func();

        Error err = cc.finalize();
        if (err != Error::kOk)
        {
            bailoutCount++;
            return false;
        }

        JitFunction fn = nullptr;
        err = codeCache.getRuntime().add(&fn, &code);
        if (err != Error::kOk)
        {
            bailoutCount++;
            return false;
        }

        codeCache.store(osrKey, fn);
        compileCount++;
        return true;
    }
}
