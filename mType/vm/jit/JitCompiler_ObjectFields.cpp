#include "JitEmissionState.hpp"
#include "JitCompiler_Objects.hpp"
#include "JitHelpers.hpp"
#include "ic/InlineCacheTable.hpp"
#include "ic/TypeFeedbackCollector.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include <asmjit/x86.h>
#include <cstddef>
#include <cstdlib>
#include <cstdint>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;

    static bool emitPushStringOpImpl(JitEmissionState& s,
                                      const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t constIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        Gp pPtr = cc.new_gp64();
        cc.mov(pPtr, s.progPtr);
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(constIndex));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_push_string),
                  FuncSignature::build<void, value::Value*,
                      const vm::bytecode::BytecodeProgram*, uint32_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, pPtr);
        inv->set_arg(2, idx);
        s.slotTypes.push_back(SlotType::STRING);
        s.stackDepth++;
        return true;
    }

    bool emitPushStringOp(JitEmissionState& s,
                          const bytecode::BytecodeProgram::Instruction& instr)
    {
        return emitPushStringOpImpl(s, instr);
    }

    // Helper-call slow path for GET_FIELD. Used both by emitGetFieldOp (when
    // the inline IC path can't fire) and as the fall-through tail of
    // tryEmitInlinedFieldGet on shape miss / null data pointer / non-primitive
    // field tag.
    static void emitGetFieldHelperInvoke(
        JitEmissionState& s,
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t fieldNameIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        Gp objAddr = cc.new_gp64();
        cc.lea(objAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
        Gp ipReg = cc.new_gp64();
        cc.mov(ipReg, static_cast<int64_t>(s.currentIP));
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(fieldNameIndex));
        Gp flagsReg = cc.new_gp64();
        cc.mov(flagsReg, static_cast<int64_t>(instr.flags));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_get_field_ic),
                  FuncSignature::build<void, value::Value*, const value::Value*,
                      JitContext*, size_t, uint32_t, uint8_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, objAddr);
        inv->set_arg(2, s.ctxPtr);
        inv->set_arg(3, ipReg);
        inv->set_arg(4, idx);
        inv->set_arg(5, flagsReg);

        // Mirror primitive payload from boxedBase to stackBase so primitive
        // consumers (NOT/JUMP_IF_FALSE/JUMP_IF_TRUE/arith) read the right
        // value. jit_unbox_int returns 0 for non-numeric variants — harmless;
        // boxed-mode consumers re-read variant from boxedBase anyway. One
        // mirror per field read costs less than per-consumer unbox.
        Gp unboxAddr = cc.new_gp64();
        cc.lea(unboxAddr, Mem(s.boxedBase,
                              static_cast<int32_t>((s.stackDepth - 1) * valueSize)));
        InvokeNode* unbox;
        cc.invoke(Out(unbox), reinterpret_cast<uint64_t>(jit_unbox_int),
                  FuncSignature::build<int64_t, const value::Value*>());
        unbox->set_arg(0, unboxAddr);
        Gp unboxed = cc.new_gp64();
        unbox->set_ret(0, unboxed);
        cc.mov(Mem(s.stackBase, (s.stackDepth - 1) * 8), unboxed);
    }

    // Speculative-inline GET_FIELD when the field IC is in monomorphic state
    // with a known shape + fieldIndex. Mirrors the method-IC inline pattern.
    //
    // Fast path: shape guard → call jit_field_data_const for the receiver's
    // field-vector data → null check → primitive-tag check → release the
    // receiver slot's refcount → 16-byte movdqu copy of the field Value into
    // the same slot → bump hit counter → join.
    //
    // Slow path: bump miss counter → emit the existing helper-call. Returns
    // false on cold IC, POLY/MEGA, or unboxed mode — caller falls back to
    // emitGetFieldHelperInvoke alone.
    static bool tryEmitInlinedFieldGet(
        JitEmissionState& s,
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (!s.typeFeedback) return false;
        if (!s.usesBoxedTypes) return false;
        if (!s.inlineFieldICHits || !s.inlineFieldICMisses) return false;

        // Opt-in disable: every GET_FIELD goes through emitGetFieldHelperInvoke
        // when set. Used to test whether the depth-2 hang originated inside
        // this inline path (raw 16-byte memcpy bypassing access validation
        // and the IC's runtime correctness checks).
        static const bool disableInlineFieldGet = []() {
            const char* v = std::getenv("MTYPE_DISABLE_INLINE_FIELD_GET");
            return v && v[0] == '1' && v[1] == '\0';
        }();
        if (disableInlineFieldGet) return false;

        auto& icTable = s.typeFeedback->getICTable();
        if (!icTable.hasFieldIC(s.currentIP)) return false;

        auto& cache = icTable.getFieldIC(s.currentIP);
        if (cache.state != ic::ICState::MONOMORPHIC) return false;
        if (cache.entryCount == 0) return false;

        const auto* shape = cache.entries[0].shape;
        size_t fieldIndex = cache.entries[0].fieldIndex;
        if (!shape || fieldIndex == SIZE_MAX) return false;

        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        const int receiverIdx = s.stackDepth - 1;

        Label slowLabel = cc.new_label();
        Label joinLabel = cc.new_label();

        // 1. Shape guard. On mismatch / null bridge / wrong variant → slow.
        emitInlineShapeGuard(s, receiverIdx, shape, slowLabel);

        // 2. Get the field-vector data pointer. Returns nullptr on lazy
        //    ObjectInstance (fieldVector not yet allocated) — slow path.
        Gp receiverAddr = cc.new_gp64();
        cc.lea(receiverAddr, Mem(s.boxedBase,
                                 static_cast<int32_t>(receiverIdx * valueSize)));
        Gp dataPtr = cc.new_gp64();
        InvokeNode* dataInv;
        cc.invoke(Out(dataInv), reinterpret_cast<uint64_t>(jit_field_data_const),
                  FuncSignature::build<const value::Value*, const value::Value*>());
        dataInv->set_arg(0, receiverAddr);
        dataInv->set_ret(0, dataPtr);
        cc.test(dataPtr, dataPtr);
        cc.jz(slowLabel);

        // 3. Source = dataPtr + fieldIndex * sizeof(Value).
        Gp srcAddr = cc.new_gp64();
        cc.lea(srcAddr, Mem(dataPtr,
                            static_cast<int32_t>(fieldIndex * valueSize)));

        // 4. Inline only when the field tag is a small primitive (INT/FLOAT/
        //    BOOL = 0/1/2). VOID/NULL_TYPE and heap tags fall to the slow
        //    path so the helper handles refcount / variant-correct assignment.
        Gp tagReg = cc.new_gp32();
        cc.movzx(tagReg, byte_ptr(srcAddr, 0));
        cc.cmp(tagReg, static_cast<int32_t>(value::ValueType::BOOL));
        cc.ja(slowLabel);

        // 5. Release the receiver slot's refcount before overwriting it.
        emitValueDestroy(s, receiverIdx);

        // 6. Raw 16-byte copy of the (primitive) field Value into the slot.
        Gp destAddr = cc.new_gp64();
        cc.lea(destAddr, Mem(s.boxedBase,
                             static_cast<int32_t>(receiverIdx * valueSize)));
        Vec tmp = cc.new_xmm();
        cc.movdqu(tmp, xmmword_ptr(srcAddr, 0));
        cc.movdqu(xmmword_ptr(destAddr, 0), tmp);

        // Mirror primitive variant payload to stackBase[receiverIdx] so
        // primitive consumers see the unboxed value. The tag-check at step 4
        // restricted to INT/FLOAT/BOOL.
        //   - INT/FLOAT (tags 0/1): payload is a raw int64 / double at offset
        //     8 — read 8 bytes.
        //   - BOOL (tag 2): payload is a 1-byte bool at offset 8; bytes 9..15
        //     are uninitialized union memory that may carry garbage from a
        //     previous variant occupant of the same slot. Pre-fix this site
        //     read 8 bytes unconditionally; the high 7 bytes of garbage made
        //     stackBase non-zero, so JUMP_IF_FALSE consistently took the
        //     `true` branch after OSR tier-up promoted the IC.
        // Branch on the tag and zero-extend the byte for BOOL; the 8-byte
        // read is correct for INT and FLOAT.
        Label readBool = cc.new_label();
        Label payloadDone = cc.new_label();
        Gp primPayload = cc.new_gp64();
        Gp tagCheck = cc.new_gp32();
        cc.movzx(tagCheck, byte_ptr(srcAddr, 0));
        cc.cmp(tagCheck, static_cast<int32_t>(value::ValueType::BOOL));
        cc.je(readBool);
        cc.mov(primPayload, qword_ptr(srcAddr, 8));
        cc.jmp(payloadDone);
        cc.bind(readBool);
        // movzx r32, byte: the mov-to-r32 implicitly zeros the upper 32 bits
        // of the underlying 64-bit register, giving a clean 0 or 1.
        cc.movzx(primPayload.r32(), byte_ptr(srcAddr, 8));
        cc.bind(payloadDone);
        cc.mov(Mem(s.stackBase, receiverIdx * 8), primPayload);

        // 7. Bump the inline-hit counter (address baked at compile time).
        Gp hitsAddr = cc.new_gp64();
        cc.mov(hitsAddr, reinterpret_cast<uint64_t>(s.inlineFieldICHits));
        cc.inc(qword_ptr(hitsAddr));

        cc.jmp(joinLabel);

        // 8. Slow path. Bump miss counter, then run the original helper.
        cc.bind(slowLabel);
        Gp missesAddr = cc.new_gp64();
        cc.mov(missesAddr, reinterpret_cast<uint64_t>(s.inlineFieldICMisses));
        cc.inc(qword_ptr(missesAddr));
        emitGetFieldHelperInvoke(s, instr);

        cc.bind(joinLabel);

        popType(s);
        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }

    bool emitGetFieldOp(JitEmissionState& s,
                        const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (tryEmitInlinedFieldGet(s, instr))
            return true;

        emitGetFieldHelperInvoke(s, instr);
        popType(s);
        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }

    // Helper-call slow path for SET_FIELD without slotType / stackDepth
    // management. Used both by the original emitSetFieldOp fall-through and
    // as the miss-branch tail of tryEmitInlinedFieldSet. Mirrors
    // emitGetFieldHelperInvoke's split.
    //
    // Pre-condition: the emitter has already popped the two input slot types;
    // stackDepth still points past the new-value slot so (stackDepth - 1) =
    // new value and (stackDepth - 2) = object/dest. Caller provides
    // newValAddr (already boxed if needed) so boxing is emitted once even
    // when this invoke runs on the miss branch of tryEmitInlinedFieldSet.
    static void emitSetFieldHelperInvoke(
        JitEmissionState& s,
        const bytecode::BytecodeProgram::Instruction& instr,
        Gp newValAddr)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        uint32_t fieldNameIndex = static_cast<uint32_t>(instr.inlineOperands[0]);
        Gp objAddr = cc.new_gp64();
        cc.lea(objAddr, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
        Gp dest = cc.new_gp64();
        cc.lea(dest, Mem(s.boxedBase, static_cast<int32_t>((s.stackDepth - 2) * valueSize)));
        Gp ipReg = cc.new_gp64();
        cc.mov(ipReg, static_cast<int64_t>(s.currentIP));
        Gp idx = cc.new_gp64();
        cc.mov(idx, static_cast<int64_t>(fieldNameIndex));
        Gp flagsReg = cc.new_gp64();
        cc.mov(flagsReg, static_cast<int64_t>(instr.flags));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_set_field_ic),
                  FuncSignature::build<void, value::Value*, const value::Value*,
                      const value::Value*,
                      JitContext*, size_t, uint32_t, uint8_t>());
        inv->set_arg(0, dest);
        inv->set_arg(1, objAddr);
        inv->set_arg(2, newValAddr);
        inv->set_arg(3, s.ctxPtr);
        inv->set_arg(4, ipReg);
        inv->set_arg(5, idx);
        inv->set_arg(6, flagsReg);
    }

    // Speculative-inline SET_FIELD when the field IC is in monomorphic state
    // with a non-value-class shape. Mirrors tryEmitInlinedFieldGet's structure.
    //
    // Fast path: shape guard → invoke jit_field_set_at (thin helper that
    // runs ObjectInstance::setFieldByIndex with its full write barrier and
    // mirrors `*dest = *newValue`) → bump hit counter.
    // Slow path: bump miss counter → emitSetFieldHelperInvoke.
    //
    // ValueClass receivers are rejected at emit time because ValueObject SET
    // is fundamentally CoW (deepCopy + reassign). A MONO IC pins one shape,
    // so the compile-time flag is authoritative.
    static bool tryEmitInlinedFieldSet(
        JitEmissionState& s,
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (!s.typeFeedback) return false;
        if (!s.usesBoxedTypes) return false;
        if (!s.inlineFieldSetICHits || !s.inlineFieldSetICMisses) return false;

        static const bool disableInlineFieldSet = []() {
            const char* v = std::getenv("MTYPE_DISABLE_INLINE_FIELD_SET");
            return v && v[0] == '1' && v[1] == '\0';
        }();
        if (disableInlineFieldSet) return false;

        auto& icTable = s.typeFeedback->getICTable();
        if (!icTable.hasFieldIC(s.currentIP)) return false;

        auto& cache = icTable.getFieldIC(s.currentIP);
        if (cache.state != ic::ICState::MONOMORPHIC) return false;
        if (cache.entryCount == 0) return false;

        const auto* shape = cache.entries[0].shape;
        size_t fieldIndex = cache.entries[0].fieldIndex;
        if (!shape || fieldIndex == SIZE_MAX) return false;
        if (shape->isValueClass()) return false;

        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        SlotType valType = popType(s);
        popType(s);
        const int receiverIdx = s.stackDepth - 2;
        const int newValIdx = s.stackDepth - 1;

        Label slowLabel = cc.new_label();
        Label joinLabel = cc.new_label();

        // 1. Box new value once (shared by both paths). In boxed-types mode
        //    the new value may still be a raw primitive slot type, so
        //    emitGetBoxedValueAddr will materialise a boxed Value into
        //    callArgs; we want that to happen exactly once.
        Gp newValAddr = emitGetBoxedValueAddr(s, newValIdx, valType);

        // 2. Shape guard. On mismatch / null bridge / wrong variant → slow.
        emitInlineShapeGuard(s, receiverIdx, shape, slowLabel);

        // 3. Invoke jit_field_set_at(destSlot, receiverSlot, fieldIndex,
        //    newValue). destSlot and receiverSlot alias the same stack
        //    position (stackDepth-2) — the helper runs setFieldByIndex before
        //    overwriting it. A false return (null bridge at runtime,
        //    unexpected ValueObject tag, out-of-range index) branches to the
        //    slow path.
        Gp slotAddr = cc.new_gp64();
        cc.lea(slotAddr, Mem(s.boxedBase,
                             static_cast<int32_t>(receiverIdx * valueSize)));
        Gp fieldIdxReg = cc.new_gp64();
        cc.mov(fieldIdxReg, static_cast<int64_t>(fieldIndex));

        Gp retReg = cc.new_gp64();
        InvokeNode* setInv;
        cc.invoke(Out(setInv), reinterpret_cast<uint64_t>(jit_field_set_at),
                  FuncSignature::build<bool, value::Value*, const value::Value*,
                      size_t, const value::Value*>());
        setInv->set_arg(0, slotAddr);
        setInv->set_arg(1, slotAddr);
        setInv->set_arg(2, fieldIdxReg);
        setInv->set_arg(3, newValAddr);
        setInv->set_ret(0, retReg);
        // bool returns land in AL; test the low byte so the upper bits
        // (undefined under the MS x64 ABI) don't affect the branch.
        cc.test(retReg.r8(), retReg.r8());
        cc.jz(slowLabel);

        // 4. Fast-path hit. Bump counter, skip the slow-path helper.
        Gp hitsAddr = cc.new_gp64();
        cc.mov(hitsAddr, reinterpret_cast<uint64_t>(s.inlineFieldSetICHits));
        cc.inc(qword_ptr(hitsAddr));
        cc.jmp(joinLabel);

        // 5. Slow path. Bump miss counter, then run jit_set_field_ic.
        cc.bind(slowLabel);
        Gp missesAddr = cc.new_gp64();
        cc.mov(missesAddr, reinterpret_cast<uint64_t>(s.inlineFieldSetICMisses));
        cc.inc(qword_ptr(missesAddr));
        emitSetFieldHelperInvoke(s, instr, newValAddr);

        cc.bind(joinLabel);

        // 6. Both paths leave the dest slot (stackDepth-2) holding newValue;
        //    destroy the original newValue slot at stackDepth-1 so its heap
        //    ref is released, then collapse the stack by one.
        if (isBoxedSlotType(valType))
            emitValueDestroy(s, s.stackDepth - 1);
        s.stackDepth--;
        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }

    bool emitSetFieldOp(JitEmissionState& s,
                        const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (tryEmitInlinedFieldSet(s, instr))
            return true;

        SlotType valType = popType(s);
        popType(s);
        Gp newValAddr = emitGetBoxedValueAddr(s, s.stackDepth - 1, valType);
        emitSetFieldHelperInvoke(s, instr, newValAddr);
        if (isBoxedSlotType(valType))
            emitValueDestroy(s, s.stackDepth - 1);
        s.stackDepth--;
        s.slotTypes.push_back(SlotType::BOXED);
        return true;
    }
}
