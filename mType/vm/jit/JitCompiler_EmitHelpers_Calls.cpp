#include "JitEmissionState.hpp"
#include "JitCompiler_EmitHelpers_Internal.hpp"
#include <asmjit/x86.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include "JitCodeCache.hpp"
#include "JitHelpers.hpp"
#include "../../value/ValueShim.hpp"

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;

    // Copy receiver + args from the caller's boxed operand stack into the
    // inlined callee's local window. MYT-169 (Fixes B + C) replaces the
    // previous per-type unbox/rebox helper-call chain with a unified raw
    // byte-level memcpy of the Value struct. For BOXED params the source
    // slot's variant tag is reset to monostate so ownership donation doesn't
    // cost a refcount bump; a matching destroy at the inline body's exit
    // (emitInlineLocalDestroy) releases the donated ownership. Primitive
    // params (int/float/bool) skip the tag reset since their Value
    // alternatives have trivial destructors.
    //
    // Records the resulting slot type into s.localTypes so subsequent
    // LOAD_LOCAL emits take the correct path.
    //
    // Source slots: [receiverStackIdx .. receiverStackIdx + argCount] in the
    // caller's boxed operand stack.
    // Destination slots: [localsBaseSlot .. localsBaseSlot + argCount] in the
    // caller's locals area.
    //
    // Only the boxed-mode path is emitted: CALL_METHOD forces usesBoxedTypes
    // true (scanOpcodesForBoxedTypes), so inline sites always run in boxed
    // mode. An unboxed-mode caller never emits CALL_METHOD in the first place.
    void emitInlineLocalCopy(JitEmissionState& s, int receiverStackIdx,
                             size_t localsBaseSlot,
                             const bytecode::BytecodeProgram::FunctionMetadata& callee,
                             bool materialiseValueReceiver)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        static_assert(valueSize % 8 == 0,
                      "Fix B raw-memcpy assumes sizeof(Value) is 8-byte aligned");
        constexpr size_t numQWords = valueSize / 8;

        const size_t total = callee.parameterCount;  // includes implicit `this`
        for (size_t i = 0; i < total; ++i)
        {
            // parameterTypes[0] is `this`'s class name for instance methods —
            // classified as BOXED by the default below. Subsequent entries are
            // user param types, where "int"/"float"/"bool" trigger the
            // primitive path. Mirrors emitArgumentUnboxing in Core.
            SlotType paramType = SlotType::INT;
            if (i < callee.parameterTypes.size())
            {
                const std::string& t = callee.parameterTypes[i];
                if      (t == "float") paramType = SlotType::FLOAT;
                else if (t == "bool")  paramType = SlotType::BOOL;
                else if (t != "int")   paramType = SlotType::BOXED;
            }
            else
            {
                paramType = SlotType::BOXED;
            }

            // MYT-185/186/187: the caller's actual push slotType decides where
            // the value physically lives. In boxed-mode emission:
            //   - primitive (INT/FLOAT/BOOL) -> stackBase via emitLoadLocal's
            //     unbox path (ControlFlow L49-72); boxedBase slot is stale.
            //   - BOXED-family -> boxedBase via jit_value_copy.
            // The callee's declared paramType MUST match the caller's push
            // slotType (bytecode-compiler invariant). The previous
            // implementation raw-memcpy'd from boxedBase unconditionally,
            // leaving primitive locals filled with stale boxedBase bytes →
            // LOAD_LOCAL in the callee read garbage via jit_unbox_int and
            // consistently returned 0. Dispatch now: primitives box into the
            // local slot from stackBase; BOXED values use the existing raw
            // memcpy + tag-reset donation path.
            const int srcSlot = receiverStackIdx + static_cast<int>(i);
            const SlotType srcType =
                (srcSlot >= 0 && static_cast<size_t>(srcSlot) < s.slotTypes.size())
                    ? s.slotTypes[static_cast<size_t>(srcSlot)]
                    : SlotType::BOXED;

            Gp dst = cc.new_gp64();
            cc.lea(dst, Mem(s.localsBase,
                            static_cast<int32_t>((localsBaseSlot + i) * s.localStride)));

            if (isBoxedSlotType(paramType))
            {
                // BOXED param: raw memcpy from boxedBase + donate ownership by
                // resetting source variant tag to monostate. The matching
                // destroy runs at emitInlineLocalDestroy.
                Gp src = cc.new_gp64();
                cc.lea(src, Mem(s.boxedBase,
                                static_cast<int32_t>(srcSlot * valueSize)));

                // MYT-346: value-class receiver requires a temp ObjectInstance
                // in local-0 so the inlined body's GET_FIELD / SET_FIELD
                // operate on a normal OBJECT-tagged Value. The raw memcpy below
                // would otherwise donate the VALUE_OBJECT bytes verbatim and
                // the inlined SET_FIELD_CACHED would route through
                // setFieldOnValueObject's CoW path (which writes to the
                // operand-stack slot of LOAD_LOCAL, not to local-0).
                //
                // The donation invariant differs here: the raw-memcpy path
                // transfers ownership via byte-copy + tag-reset (zero ref
                // count delta). The materialise path reads the source without
                // taking ownership, so the source's shared_ptr<ValueObject>
                // ref count must be released by jit_value_destroy (which also
                // leaves the slot in monostate, matching the tag-reset's
                // post-condition).
                if (materialiseValueReceiver && i == 0)
                {
                    InvokeNode* matInv;
                    cc.invoke(Out(matInv),
                              reinterpret_cast<uint64_t>(
                                  jit_materialise_value_receiver_into_local),
                              FuncSignature::build<void, value::Value*,
                                                   const value::Value*>());
                    matInv->set_arg(0, dst);
                    matInv->set_arg(1, src);

                    InvokeNode* destroyInv;
                    cc.invoke(Out(destroyInv),
                              reinterpret_cast<uint64_t>(jit_value_destroy),
                              FuncSignature::build<void, value::Value*>());
                    destroyInv->set_arg(0, src);
                }
                else
                {
                    Gp scratch = cc.new_gp64();
                    for (size_t k = 0; k < numQWords; ++k)
                    {
                        cc.mov(scratch, qword_ptr(src, static_cast<int32_t>(k * 8)));
                        cc.mov(qword_ptr(dst, static_cast<int32_t>(k * 8)), scratch);
                    }
                    const auto& layout = detail::getInlineShapeLayout();
                    cc.mov(byte_ptr(src, static_cast<int32_t>(layout.tagOffset)),
                            static_cast<int32_t>(layout.voidTag));
                }
            }
            else
            {
                // Primitive param: box the caller's stackBase raw primitive
                // into the callee's local slot as a valid Value. LOAD_LOCAL
                // in the callee body reads this via jit_unbox_* and gets the
                // correct primitive. No destroy needed (trivial dtor).
                if (paramType == SlotType::FLOAT)
                {
                    Vec srcVal = cc.new_xmm();
                    if (srcType == SlotType::FLOAT)
                    {
                        cc.movsd(srcVal, Mem(s.stackBase, srcSlot * 8));
                    }
                    else
                    {
                        Gp srcAddr = emitGetBoxedValueAddr(s, srcSlot, srcType);
                        InvokeNode* unbox;
                        cc.invoke(Out(unbox), reinterpret_cast<uint64_t>(jit_unbox_float),
                                  FuncSignature::build<double, const value::Value*>());
                        unbox->set_arg(0, srcAddr);
                        unbox->set_ret(0, srcVal);
                    }
                    InvokeNode* inv;
                    cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_box_float),
                              FuncSignature::build<void, value::Value*, double>());
                    inv->set_arg(0, dst);
                    inv->set_arg(1, srcVal);
                }
                else
                {
                    Gp srcVal = cc.new_gp64();
                    if (!isBoxedSlotType(srcType) && srcType != SlotType::FLOAT)
                    {
                        cc.mov(srcVal, Mem(s.stackBase, srcSlot * 8));
                    }
                    else
                    {
                        Gp srcAddr = emitGetBoxedValueAddr(s, srcSlot, srcType);
                        InvokeNode* unbox;
                        cc.invoke(Out(unbox), reinterpret_cast<uint64_t>(jit_unbox_int),
                                  FuncSignature::build<int64_t, const value::Value*>());
                        unbox->set_arg(0, srcAddr);
                        unbox->set_ret(0, srcVal);
                    }
                    InvokeNode* inv;
                    const uint64_t fn = (paramType == SlotType::BOOL)
                        ? reinterpret_cast<uint64_t>(jit_box_bool)
                        : reinterpret_cast<uint64_t>(jit_box_int);
                    cc.invoke(Out(inv), fn,
                              FuncSignature::build<void, value::Value*, int64_t>());
                    inv->set_arg(0, dst);
                    inv->set_arg(1, srcVal);
                }
            }

            s.localTypes[localsBaseSlot + i] = paramType;
        }
    }

    // MYT-185/186/187: physical-state fix. The slow path's
    // emitReturnValueCopyBoxed populates BOTH boxedBase[receiverStackIdx]
    // (via jit_value_copy from ctx->returnValue) AND
    // stackBase[receiverStackIdx] (via jit_unbox_int mirror), and pushes
    // slotTypes BOXED. Compile-time state at endLabel therefore expects top
    // = BOXED with both slots valid. The fast path's callee body in boxed-
    // mode emission leaves the return value in exactly one of the two:
    // INT/BOOL primitives live in stackBase (emitLoadLocal unboxes into
    // stackBase; ADD_INT / arithmetic produce INT in stackBase); BOXED
    // family values (GET_FIELD results, string concats, CALL_METHOD returns)
    // live in boxedBase. This helper converges both slots so the endLabel
    // join sees matching physical state regardless of which path ran.
    void emitInlineReturnMaterialize(JitEmissionState& s, int receiverStackIdx,
                                     SlotType returnSlotType)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        if (isBoxedSlotType(returnSlotType))
        {
            // Value in boxedBase; mirror to stackBase. jit_unbox_int returns
            // 0 for non-numeric Value variants (harmless, matches slow path's
            // behaviour for string/object returns).
            Gp addr = cc.new_gp64();
            cc.lea(addr, Mem(s.boxedBase,
                             static_cast<int32_t>(receiverStackIdx * valueSize)));
            InvokeNode* unbox;
            cc.invoke(Out(unbox), reinterpret_cast<uint64_t>(jit_unbox_int),
                      FuncSignature::build<int64_t, const value::Value*>());
            unbox->set_arg(0, addr);
            Gp unboxed = cc.new_gp64();
            unbox->set_ret(0, unboxed);
            cc.mov(Mem(s.stackBase, receiverStackIdx * 8), unboxed);
            return;
        }

        // Primitive: value in stackBase; box into boxedBase.
        Gp destAddr = cc.new_gp64();
        cc.lea(destAddr, Mem(s.boxedBase,
                             static_cast<int32_t>(receiverStackIdx * valueSize)));

        if (returnSlotType == SlotType::FLOAT)
        {
            Vec srcVal = cc.new_xmm();
            cc.movsd(srcVal, Mem(s.stackBase, receiverStackIdx * 8));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_box_float),
                      FuncSignature::build<void, value::Value*, double>());
            inv->set_arg(0, destAddr);
            inv->set_arg(1, srcVal);
        }
        else
        {
            Gp srcVal = cc.new_gp64();
            cc.mov(srcVal, Mem(s.stackBase, receiverStackIdx * 8));
            InvokeNode* inv;
            const uint64_t fn = (returnSlotType == SlotType::BOOL)
                ? reinterpret_cast<uint64_t>(jit_box_bool)
                : reinterpret_cast<uint64_t>(jit_box_int);
            cc.invoke(Out(inv), fn,
                      FuncSignature::build<void, value::Value*, int64_t>());
            inv->set_arg(0, destAddr);
            inv->set_arg(1, srcVal);
        }
    }

    // MYT-169 Fix B: release the donated shared_ptr ownership that
    // emitInlineLocalCopy's raw memcpy left aliased in each boxed inline-
    // local slot. Primitive params (INT/FLOAT/BOOL) have trivial Value
    // destructors and are skipped. jit_value_destroy leaves the slot as
    // monostate so the next inline-call iteration's memcpy overwrites
    // safely, and the final emitCleanup at function exit sees a monostate
    // (no-op ~Value).
    void emitInlineLocalDestroy(JitEmissionState& s, size_t localsBaseSlot,
                                const bytecode::BytecodeProgram::FunctionMetadata& callee)
    {
        auto& cc = s.cc;
        const size_t total = callee.parameterCount;
        for (size_t i = 0; i < total; ++i)
        {
            SlotType paramType = SlotType::INT;
            if (i < callee.parameterTypes.size())
            {
                const std::string& t = callee.parameterTypes[i];
                if      (t == "float") paramType = SlotType::FLOAT;
                else if (t == "bool")  paramType = SlotType::BOOL;
                else if (t != "int")   paramType = SlotType::BOXED;
            }
            else
            {
                paramType = SlotType::BOXED;
            }
            if (!isBoxedSlotType(paramType))
                continue;

            Gp addr = cc.new_gp64();
            cc.lea(addr, Mem(s.localsBase,
                            static_cast<int32_t>((localsBaseSlot + i) * s.localStride)));
            InvokeNode* inv;
            cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_destroy),
                      FuncSignature::build<void, value::Value*>());
            inv->set_arg(0, addr);
        }
    }

    bool finalizeAndStore(Compiler& cc, CodeHolder& code,
                          JitCodeCache& codeCache, const std::string& key,
                          size_t& compileCount, size_t& bailoutCount)
    {
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

        codeCache.store(key, fn);
        compileCount++;
        return true;
    }
}
