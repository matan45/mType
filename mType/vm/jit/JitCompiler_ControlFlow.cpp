#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include <asmjit/x86.h>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    // MYT-164 (Phase F-b): resolve a jump target against the innermost active
    // inline frame's localJumpLabels. Returns nullptr when no inline frame is
    // active or the target is outside the callee's bytecode range (in which
    // case the JUMP-family emitter falls back to the outer s.labels / onExit
    // path). Only the top frame is consulted — jumps inside an inner callee
    // cannot target an outer frame's bytecode range by lexical scope.
    static asmjit::Label* findInlineJumpLabel(JitEmissionState& s, size_t target)
    {
        if (s.inlineStack.empty()) return nullptr;
        auto& frame = s.inlineStack.back();
        auto it = frame.localJumpLabels.find(target);
        if (it == frame.localJumpLabels.end()) return nullptr;
        return &it->second;
    }

    static void emitLoadLocal(JitEmissionState& s, size_t slot)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        // MYT-163: remap the bytecode's logical slot into the caller's frame
        // when emission is inside an inlined callee. inlineLocalsBase == 0 at
        // the top level — the computation is a no-op for non-inline paths.
        const size_t physSlot = slot + s.inlineLocalsBase;
        SlotType lt = s.localTypes.count(physSlot) ? s.localTypes[physSlot] : SlotType::INT;

        if (s.usesBoxedTypes && isBoxedSlotType(lt))
        {
            Gp src = cc.new_gp64();
            cc.lea(src, Mem(s.localsBase, static_cast<int32_t>(physSlot * s.localStride)));
            Gp dst = cc.new_gp64();
            cc.lea(dst, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            inv->set_arg(0, dst);
            inv->set_arg(1, src);
        }
        else if (s.usesBoxedTypes)
        {
            Gp localAddr = cc.new_gp64();
            cc.lea(localAddr, Mem(s.localsBase, static_cast<int32_t>(physSlot * s.localStride)));
            if (lt == SlotType::FLOAT)
            {
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_float),
                          FuncSignature::build<double, const value::Value*>());
                inv->set_arg(0, localAddr);
                Vec val = cc.new_xmm();
                inv->set_ret(0, val);
                cc.movsd(Mem(s.stackBase, s.stackDepth * 8), val);
            }
            else
            {
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_unbox_int),
                          FuncSignature::build<int64_t, const value::Value*>());
                inv->set_arg(0, localAddr);
                Gp val = cc.new_gp64();
                inv->set_ret(0, val);
                cc.mov(Mem(s.stackBase, s.stackDepth * 8), val);
            }
        }
        else
        {
            Gp tmp = cc.new_gp64();
            cc.mov(tmp, Mem(s.localsBase, static_cast<int32_t>(physSlot * 8)));
            cc.mov(Mem(s.stackBase, s.stackDepth * 8), tmp);
        }
        s.slotTypes.push_back(lt);
        s.stackDepth++;
    }

    static void emitStoreLocal(JitEmissionState& s, size_t slot)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        SlotType tt = topType(s);
        // MYT-163: see emitLoadLocal — remap through inlineLocalsBase.
        const size_t physSlot = slot + s.inlineLocalsBase;

        if (s.usesBoxedTypes && isBoxedSlotType(tt))
        {
            Gp src = cc.new_gp64();
            cc.lea(src, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
            Gp dst = cc.new_gp64();
            cc.lea(dst, Mem(s.localsBase, static_cast<int32_t>(physSlot * s.localStride)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            inv->set_arg(0, dst);
            inv->set_arg(1, src);
        }
        else if (s.usesBoxedTypes)
        {
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(s.localsBase, static_cast<int32_t>(physSlot * s.localStride)));
            if (tt == SlotType::FLOAT)
            {
                Vec val = cc.new_xmm();
                cc.movsd(val, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                InvokeNode* inv;
                cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_box_float),
                          FuncSignature::build<void, value::Value*, double>());
                inv->set_arg(0, destAddr);
                inv->set_arg(1, val);
            }
            else
            {
                Gp val = cc.new_gp64();
                cc.mov(val, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                uint64_t fn = (tt == SlotType::BOOL)
                    ? reinterpret_cast<uint64_t>(jit_box_bool)
                    : reinterpret_cast<uint64_t>(jit_box_int);
                InvokeNode* inv;
                cc.invoke(Out(inv), fn,
                          FuncSignature::build<void, value::Value*, int64_t>());
                inv->set_arg(0, destAddr);
                inv->set_arg(1, val);
            }
        }
        else
        {
            Gp tmp = cc.new_gp64();
            cc.mov(tmp, Mem(s.stackBase, (s.stackDepth - 1) * 8));
            cc.mov(Mem(s.localsBase, static_cast<int32_t>(physSlot * 8)), tmp);
        }
        s.localTypes[physSlot] = tt;
    }

    // MYT-152: JIT emitter for LOAD_VAR. Globals and unqualified field lookups
    // have no compile-time primitive type, so the loaded Value is always
    // placed in the boxed operand stack. scanOpcodesForBoxedTypes forces
    // s.usesBoxedTypes = true whenever LOAD_VAR is present in the loop body;
    // the guard below bails cleanly if that invariant is ever violated.
    static void emitLoadVar(JitEmissionState& s, uint32_t nameIndex)
    {
        if (!s.usesBoxedTypes) { s.compileFailed = true; return; }
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase,
                         static_cast<int32_t>(s.stackDepth * valueSize)));
        Gp niReg = cc.new_gp64();
        cc.mov(niReg, static_cast<int64_t>(nameIndex));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_load_var),
                  FuncSignature::build<void, value::Value*, JitContext*,
                                        uint32_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, s.ctxPtr);
        inv->set_arg(2, niReg);

        s.slotTypes.push_back(SlotType::BOXED);
        s.stackDepth++;
    }

    // MYT-152: JIT emitter for STORE_VAR. The runtime pops the value, writes
    // it to the global/field, then re-pushes the same bits (so expression-
    // context `int x = (a = 5)` works). We mirror emitStoreLocal's approach:
    // pass the top slot to jit_store_var and leave stackDepth / slotTypes
    // untouched, since the bits at top-of-stack are identical before and
    // after. The statement-context POP emitted by StatementCompiler's Path B
    // consumes the re-pushed value.
    static void emitStoreVar(JitEmissionState& s, uint32_t nameIndex)
    {
        if (!s.usesBoxedTypes) { s.compileFailed = true; return; }
        auto& cc = s.cc;

        SlotType valType = topType(s);
        Gp valAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, valType);
        Gp niReg = cc.new_gp64();
        cc.mov(niReg, static_cast<int64_t>(nameIndex));

        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_store_var),
                  FuncSignature::build<void, JitContext*, uint32_t,
                                        const value::Value*>());
        inv->set_arg(0, s.ctxPtr);
        inv->set_arg(1, niReg);
        inv->set_arg(2, valAddr);
    }

    static void emitReturnPrimitive(JitEmissionState& s, SlotType retType)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        if (s.usesBoxedTypes && isBoxedSlotType(retType))
        {
            Gp retAddr = cc.new_gp64();
            cc.lea(retAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_boxed),
                      FuncSignature::build<void, JitContext*, const value::Value*>());
            inv->set_arg(0, s.ctxPtr);
            inv->set_arg(1, retAddr);
        }
        else if (retType == SlotType::FLOAT)
        {
            Vec retVal = cc.new_xmm();
            cc.movsd(retVal, Mem(s.stackBase, s.stackDepth * 8));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_float),
                      FuncSignature::build<void, JitContext*, double>());
            inv->set_arg(0, s.ctxPtr);
            inv->set_arg(1, retVal);
        }
        else if (retType == SlotType::BOOL)
        {
            Gp retVal = cc.new_gp64();
            cc.mov(retVal, Mem(s.stackBase, s.stackDepth * 8));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_bool),
                      FuncSignature::build<void, JitContext*, int64_t>());
            inv->set_arg(0, s.ctxPtr);
            inv->set_arg(1, retVal);
        }
        else
        {
            Gp retVal = cc.new_gp64();
            cc.mov(retVal, Mem(s.stackBase, s.stackDepth * 8));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_return_int),
                      FuncSignature::build<void, JitContext*, int64_t>());
            inv->set_arg(0, s.ctxPtr);
            inv->set_arg(1, retVal);
        }
    }

    static void emitReturnValueOp(JitEmissionState& s, const ExitHandler& onExit)
    {
        if (onExit)
        {
            s.stackDepth--;
            popType(s);
            onExit(s, 0);
            return;
        }

        s.stackDepth--;
        SlotType retType = popType(s);
        emitReturnPrimitive(s, retType);
        emitCleanup(s);
        s.cc.ret();
    }

    static std::string resolveCallReturnType(JitEmissionState& s,
                                              const bytecode::BytecodeProgram::Instruction& instr)
    {
        uint32_t nameIndex = static_cast<uint32_t>(instr.operands[0]);
        if (nameIndex >= s.program.getConstantPool().strings.size())
        {
            s.compileFailed = true;
            return "";
        }
        const std::string& funcName = s.program.getConstantPool().getString(nameIndex);
        const auto* calleeMeta = s.program.getFunction(funcName);

        if (calleeMeta)
            return calleeMeta->returnType;

        if (funcName == "print" || funcName == "println")
            return "void";

        s.compileFailed = true;
        return "";
    }

    static void emitCallReturnValue(JitEmissionState& s,
                                     const std::string& returnType, bool isPrimReturn)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        if (isPrimReturn)
        {
            SlotType retSlot = SlotType::INT;
            if (returnType == "float") retSlot = SlotType::FLOAT;
            else if (returnType == "bool") retSlot = SlotType::BOOL;

            Gp retAddr = cc.new_gp64();
            cc.lea(retAddr, Mem(s.ctxPtr, offsetof(JitContext, returnValue)));
            emitUnbox(s, retAddr, s.stackDepth, retSlot);
            s.slotTypes.push_back(retSlot);
        }
        else
        {
            Gp retAddr = cc.new_gp64();
            cc.lea(retAddr, Mem(s.ctxPtr, offsetof(JitContext, returnValue)));
            Gp destAddr = cc.new_gp64();
            cc.lea(destAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
            InvokeNode* cpInv;
            cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                      FuncSignature::build<void, value::Value*, const value::Value*>());
            cpInv->set_arg(0, destAddr);
            cpInv->set_arg(1, retAddr);
            s.slotTypes.push_back(SlotType::BOXED);
        }
        s.stackDepth++;
    }

    static bool emitCallOp(JitEmissionState& s,
                            const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        uint32_t nameIndex = static_cast<uint32_t>(instr.operands[0]);
        size_t argCount = instr.operands[1];

        if (argCount > JitContext::MAX_CALL_ARGS)
        {
            s.compileFailed = true;
            return true;
        }

        std::string returnType = resolveCallReturnType(s, instr);
        if (s.compileFailed) return true;

        bool isPrimReturn = (returnType == "int" || returnType == "float" ||
                             returnType == "bool" || returnType == "void");
        if (!isPrimReturn && !s.usesBoxedTypes)
        {
            s.compileFailed = true;
            return true;
        }

        emitBoxCallArgs(s, argCount);
        emitPopAndDestroyArgs(s, argCount);

        Gp niReg = cc.new_gp64();
        cc.mov(niReg, static_cast<int64_t>(nameIndex));
        Gp acReg = cc.new_gp64();
        cc.mov(acReg, static_cast<int64_t>(argCount));

        InvokeNode* callInv;
        cc.invoke(Out(callInv), reinterpret_cast<uint64_t>(jit_call_function),
                  FuncSignature::build<void, JitContext*, uint32_t, size_t>());
        callInv->set_arg(0, s.ctxPtr);
        callInv->set_arg(1, niReg);
        callInv->set_arg(2, acReg);

        if (returnType != "void")
            emitCallReturnValue(s, returnType, isPrimReturn);

        return true;
    }

    static bool emitCallFastOp(JitEmissionState& s,
                                const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        uint32_t funcIndex = static_cast<uint32_t>(instr.operands[0]);
        size_t argCount = instr.operands[1];

        if (argCount > JitContext::MAX_CALL_ARGS)
        {
            s.compileFailed = true;
            return true;
        }

        const auto* calleeMeta = s.program.getFunctionByIndex(funcIndex);
        if (!calleeMeta)
        {
            s.compileFailed = true;
            return true;
        }
        const std::string& returnType = calleeMeta->returnType;

        bool isPrimReturn = (returnType == "int" || returnType == "float" ||
                             returnType == "bool" || returnType == "void");
        if (!isPrimReturn && !s.usesBoxedTypes)
        {
            s.compileFailed = true;
            return true;
        }

        emitBoxCallArgs(s, argCount);
        emitPopAndDestroyArgs(s, argCount);

        Gp fiReg = cc.new_gp64();
        cc.mov(fiReg, static_cast<int64_t>(funcIndex));
        Gp acReg = cc.new_gp64();
        cc.mov(acReg, static_cast<int64_t>(argCount));

        InvokeNode* callInv;
        cc.invoke(Out(callInv), reinterpret_cast<uint64_t>(jit_call_function_fast),
                  FuncSignature::build<void, JitContext*, uint32_t, size_t>());
        callInv->set_arg(0, s.ctxPtr);
        callInv->set_arg(1, fiReg);
        callInv->set_arg(2, acReg);

        if (returnType != "void")
            emitCallReturnValue(s, returnType, isPrimReturn);

        return true;
    }

    bool emitControlFlowOps(JitEmissionState& s,
                            const bytecode::BytecodeProgram::Instruction& instr,
                            const ExitHandler& onExit)
    {
        auto& cc = s.cc;

        switch (instr.opcode)
        {
            case OpCode::LOAD_LOCAL:
                emitLoadLocal(s, instr.operands[0]);
                return true;

            case OpCode::STORE_LOCAL:
                emitStoreLocal(s, instr.operands[0]);
                return true;

            case OpCode::LOAD_VAR:
                emitLoadVar(s, static_cast<uint32_t>(instr.operands[0]));
                return true;

            case OpCode::STORE_VAR:
                emitStoreVar(s, static_cast<uint32_t>(instr.operands[0]));
                return true;

            case OpCode::JUMP:
            {
                size_t target = instr.operands[0];
                if (auto* lbl = findInlineJumpLabel(s, target))
                {
                    cc.jmp(*lbl);
                    return true;
                }
                if (onExit && s.labels.find(target) == s.labels.end())
                {
                    onExit(s, target);
                }
                else
                {
                    cc.jmp(s.labels[target]);
                }
                return true;
            }

            case OpCode::JUMP_IF_FALSE:
            {
                size_t target = instr.operands[0];
                s.stackDepth--;
                popType(s);
                Gp cond = cc.new_gp64();
                cc.mov(cond, Mem(s.stackBase, s.stackDepth * 8));
                cc.test(cond, cond);

                if (auto* lbl = findInlineJumpLabel(s, target))
                {
                    cc.jz(*lbl);
                }
                else if (onExit && s.labels.find(target) == s.labels.end())
                {
                    Label continueLoop = cc.new_label();
                    cc.jnz(continueLoop);
                    onExit(s, target);
                    cc.bind(continueLoop);
                }
                else
                {
                    cc.jz(s.labels[target]);
                }
                return true;
            }

            case OpCode::JUMP_IF_TRUE:
            {
                size_t target = instr.operands[0];
                s.stackDepth--;
                popType(s);
                Gp cond = cc.new_gp64();
                cc.mov(cond, Mem(s.stackBase, s.stackDepth * 8));
                cc.test(cond, cond);

                if (auto* lbl = findInlineJumpLabel(s, target))
                {
                    cc.jnz(*lbl);
                }
                else if (onExit && s.labels.find(target) == s.labels.end())
                {
                    Label continueLoop = cc.new_label();
                    cc.jz(continueLoop);
                    onExit(s, target);
                    cc.bind(continueLoop);
                }
                else
                {
                    cc.jnz(s.labels[target]);
                }
                return true;
            }

            case OpCode::JUMP_IF_FALSE_OR_POP:
            case OpCode::JUMP_IF_TRUE_OR_POP:
            {
                // Short-circuit semantics: on jump-taken the condition value is
                // preserved on the runtime stack (becomes the expression result);
                // on fall-through the value is popped. Peek without decrementing
                // the runtime-side counter before the conditional jump.
                size_t target = instr.operands[0];
                bool jumpOnZero = (instr.opcode == OpCode::JUMP_IF_FALSE_OR_POP);

                Gp cond = cc.new_gp64();
                cc.mov(cond, Mem(s.stackBase, (s.stackDepth - 1) * 8));
                cc.test(cond, cond);

                if (auto* lbl = findInlineJumpLabel(s, target))
                {
                    if (jumpOnZero) cc.jz(*lbl);
                    else            cc.jnz(*lbl);
                }
                else if (onExit && s.labels.find(target) == s.labels.end())
                {
                    Label continueLoop = cc.new_label();
                    if (jumpOnZero) cc.jnz(continueLoop);
                    else            cc.jz(continueLoop);
                    onExit(s, target);
                    cc.bind(continueLoop);
                }
                else
                {
                    if (jumpOnZero) cc.jz(s.labels[target]);
                    else            cc.jnz(s.labels[target]);
                }

                // Fall-through path: pop the condition value. The bytecode
                // compiler guarantees BOOL/INT primitive on top at short-circuit
                // sites (ExpressionCompiler.cpp:63-68, 92-97), so no boxed
                // destroy is needed — mirrors the existing JUMP_IF_FALSE/TRUE
                // primitive-only contract.
                popType(s);
                s.stackDepth--;
                return true;
            }

            case OpCode::JUMP_BACK:
            {
                InvokeNode* gc;
                cc.invoke(Out(gc), reinterpret_cast<uint64_t>(jit_gc_safepoint),
                          FuncSignature::build<void>());
                size_t target = instr.operands[0];
                if (auto* lbl = findInlineJumpLabel(s, target))
                    cc.jmp(*lbl);
                else
                    cc.jmp(s.labels[target]);
                return true;
            }

            case OpCode::RETURN:
            {
                if (onExit)
                {
                    onExit(s, 0);
                }
                else
                {
                    emitCleanup(s);
                    cc.mov(byte_ptr(s.ctxPtr, offsetof(JitContext, hasReturnValue)), 0);
                    cc.ret();
                }
                return true;
            }

            case OpCode::RETURN_VALUE:
                emitReturnValueOp(s, onExit);
                return true;

            case OpCode::CALL:
                return emitCallOp(s, instr);

            case OpCode::CALL_FAST:
                return emitCallFastOp(s, instr);

            default:
                return false;
        }
    }
}
