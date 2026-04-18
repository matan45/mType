#include "JitEmissionState.hpp"
#include "JitHelpers.hpp"
#include "JitCodeCache.hpp"
#include "../bytecode/OpCode.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../value/ValueObject.hpp"
#include <asmjit/x86.h>
#include <cassert>
#include <cstdint>
#include <memory>

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

    namespace {
        // MYT-169: layout constants consumed by the inlined shape-guard emitter.
        // Computed once at first use; cached in a function-local static.
        struct InlineShapeLayout
        {
            size_t variantTagOffset;        // byte offset of discriminant in value::Value
            size_t objInstClassDefOffset;   // ObjectInstance → classDefinition shared_ptr
            size_t valueObjClassDefOffset;  // ValueObject    → classDefinition shared_ptr
            uint8_t objInstAltTag;          // discriminant byte value for shared_ptr<ObjectInstance>
            uint8_t valueObjAltTag;         // discriminant byte value for shared_ptr<ValueObject>
            uint8_t monostateAltTag;        // discriminant byte value for std::monostate (trivial dtor)
        };

        const InlineShapeLayout& getInlineShapeLayout()
        {
            static const InlineShapeLayout layout = []() {
                // Probe the std::variant discriminant byte position: construct two
                // Values with known adjacent alternative indices (int64_t → 0,
                // double → 1) and find the byte that flips 0 → 1. MSVC stores a
                // single-byte _Which at the tail of the variant storage.
                value::Value v0{int64_t{0}};
                value::Value v1{double{0.0}};
                const uint8_t* b0 = reinterpret_cast<const uint8_t*>(&v0);
                const uint8_t* b1 = reinterpret_cast<const uint8_t*>(&v1);
                size_t tagOffset = sizeof(value::Value) - 1;
                for (size_t i = sizeof(value::Value); i-- > 0;)
                {
                    if (b0[i] == 0 && b1[i] == 1) { tagOffset = i; break; }
                }

                // Probe alt tags for the receiver-bearing alternatives and for
                // std::monostate (used by MYT-169 Fix B as a no-op-destructor tag
                // when donating receiver ownership from operand stack to local).
                value::Value vObj{std::shared_ptr<runtimeTypes::klass::ObjectInstance>{}};
                value::Value vVal{std::shared_ptr<value::ValueObject>{}};
                value::Value vMono{std::monostate{}};

                InlineShapeLayout l;
                l.variantTagOffset       = tagOffset;
                l.objInstAltTag          = reinterpret_cast<const uint8_t*>(&vObj)[tagOffset];
                l.valueObjAltTag         = reinterpret_cast<const uint8_t*>(&vVal)[tagOffset];
                l.monostateAltTag        = reinterpret_cast<const uint8_t*>(&vMono)[tagOffset];
                l.objInstClassDefOffset  = runtimeTypes::klass::ObjectInstance::classDefinitionMemberOffset();
                l.valueObjClassDefOffset = value::ValueObject::classDefinitionMemberOffset();

                // Sanity: alt tags must match the variant alternative order in
                // ValueType.hpp (ObjectInstance at index 6, ValueObject at 7,
                // monostate at 5).
                assert(l.objInstAltTag == 6 && l.valueObjAltTag == 7 && l.monostateAltTag == 5);
                return l;
            }();
            return layout;
        }
    }

    void emitValueDestroy(JitEmissionState& s, int slotOffset)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        Gp addr = cc.new_gp64();
        cc.lea(addr, Mem(s.boxedBase, static_cast<int32_t>(slotOffset * valueSize)));
        InvokeNode* inv;
        cc.invoke(Out(inv), reinterpret_cast<uint64_t>(jit_value_destroy),
                  FuncSignature::build<void, value::Value*>());
        inv->set_arg(0, addr);
    }

    void emitReturnValueCopyBoxed(JitEmissionState& s)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        Gp retAddr = cc.new_gp64();
        cc.lea(retAddr, Mem(s.ctxPtr, offsetof(JitContext, returnValue)));
        Gp destAddr = cc.new_gp64();
        cc.lea(destAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        InvokeNode* cpInv;
        cc.invoke(Out(cpInv), reinterpret_cast<uint64_t>(jit_value_copy),
                  FuncSignature::build<void, value::Value*, const value::Value*>());
        cpInv->set_arg(0, destAddr);
        cpInv->set_arg(1, retAddr);

        // MYT-154: also mirror the int/bool payload to the unboxed stack so
        // primitive-stack consumers (JUMP_IF_FALSE / JUMP_IF_TRUE / ADD_INT /
        // LT_INT, etc.) read the right value when the call returned a primitive.
        // jit_unbox_int returns 0 for non-numeric variants, which is harmless —
        // boxed-mode consumers re-read the variant from boxedBase anyway and
        // ignore the unboxed mirror.
        Gp unboxAddr = cc.new_gp64();
        cc.lea(unboxAddr, Mem(s.boxedBase, static_cast<int32_t>(s.stackDepth * valueSize)));
        InvokeNode* unbox;
        cc.invoke(Out(unbox), reinterpret_cast<uint64_t>(jit_unbox_int),
                  FuncSignature::build<int64_t, const value::Value*>());
        unbox->set_arg(0, unboxAddr);
        Gp unboxed = cc.new_gp64();
        unbox->set_ret(0, unboxed);
        cc.mov(Mem(s.stackBase, s.stackDepth * 8), unboxed);

        s.slotTypes.push_back(SlotType::BOXED);
        s.stackDepth++;
    }

    Gp emitGetBoxedValueAddr(JitEmissionState& s, int stackIdx, SlotType valType)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        Gp addr = cc.new_gp64();
        if (!isBoxedSlotType(valType))
        {
            cc.lea(addr, Mem(s.ctxPtr, offsetof(JitContext, callArgs)));
            emitBox(s, addr, stackIdx, valType);
        }
        else
        {
            cc.lea(addr, Mem(s.boxedBase, static_cast<int32_t>(stackIdx * valueSize)));
        }
        return addr;
    }

    bool scanOpcodesForBoxedTypes(const bytecode::BytecodeProgram& program,
                                  size_t startOffset, size_t endOffset)
    {
        for (size_t ip = startOffset; ip < endOffset; ++ip)
        {
            const auto& si = program.getInstruction(ip);
            switch (si.opcode)
            {
                case OpCode::PUSH_STRING: case OpCode::GET_FIELD:
                case OpCode::SET_FIELD:   case OpCode::INLINE_SET_FIELD:
                // MYT-152: LOAD_VAR / STORE_VAR produce / consume boxed Values
                // (global or field lookups have no compile-time primitive
                // type), so the enclosing loop must emit in boxed mode.
                case OpCode::LOAD_VAR:    case OpCode::STORE_VAR:
                case OpCode::NEW_OBJECT:
                case OpCode::NEW_VALUE_OBJECT: case OpCode::OBJECT_TO_VALUE:
                case OpCode::CALL_METHOD: case OpCode::CALL_STATIC:
                case OpCode::NEW_ARRAY:   case OpCode::NEW_ARRAY_MULTI:
                case OpCode::ARRAY_GET:
                case OpCode::ARRAY_SET:   case OpCode::ARRAY_LENGTH:
                case OpCode::ARRAY_GET_INT_LOCAL: case OpCode::ARRAY_SET_INT_LOCAL:
                case OpCode::ARRAY_LENGTH_LOCAL:
                case OpCode::INSTANCEOF:  case OpCode::INSTANCEOF_TYPEPARAM:
                case OpCode::CAST:
                // MYT-147: iterator opcodes receive / produce boxed
                // ObjectInstance values (ArrayIteratorHelper,
                // HashMapKeyIterator, LinkedListIterator, ...) on the operand
                // stack, so the enclosing function must run in boxed-types
                // mode. ITERATOR_HAS_NEXT pushes bool; ITERATOR_CLOSE pushes
                // nothing - but they're stack-adjacent to boxed iterator slots
                // and are safe to include in the boxed-mode trigger set.
                case OpCode::GET_ITERATOR:      case OpCode::ITERATOR_HAS_NEXT:
                case OpCode::ITERATOR_NEXT:     case OpCode::ITERATOR_CLOSE:
                // Specialized primitive-method opcodes receive boxed Int / Float
                // objects on the operand stack, so the enclosing function must
                // be emitted in boxed-types mode.
                case OpCode::INVOKE_INT_ADD: case OpCode::INVOKE_INT_SUB:
                case OpCode::INVOKE_INT_MUL: case OpCode::INVOKE_INT_DIV:
                case OpCode::INVOKE_INT_MOD: case OpCode::INVOKE_INT_NEG:
                case OpCode::INVOKE_INT_ABS: case OpCode::INVOKE_INT_EQUALS:
                case OpCode::INVOKE_INT_COMPARE: case OpCode::INVOKE_INT_GET_VALUE:
                case OpCode::INVOKE_INT_LESS_THAN: case OpCode::INVOKE_INT_LESS_EQUAL:
                case OpCode::INVOKE_INT_GREATER_THAN: case OpCode::INVOKE_INT_GREATER_EQUAL:
                case OpCode::INVOKE_FLOAT_ADD: case OpCode::INVOKE_FLOAT_SUB:
                case OpCode::INVOKE_FLOAT_MUL: case OpCode::INVOKE_FLOAT_DIV:
                case OpCode::INVOKE_FLOAT_NEG: case OpCode::INVOKE_FLOAT_ABS:
                case OpCode::INVOKE_FLOAT_EQUALS: case OpCode::INVOKE_FLOAT_COMPARE:
                case OpCode::INVOKE_FLOAT_GET_VALUE: case OpCode::INVOKE_BOOL_GET_VALUE:
                case OpCode::INVOKE_FLOAT_LESS_THAN: case OpCode::INVOKE_FLOAT_LESS_EQUAL:
                case OpCode::INVOKE_FLOAT_GREATER_THAN: case OpCode::INVOKE_FLOAT_GREATER_EQUAL:
                    return true;
                default: break;
            }
        }
        for (size_t ip = startOffset; ip < endOffset; ++ip)
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
                    return true;
                }
            }
        }
        return false;
    }

    std::unordered_map<size_t, Label> createJumpLabels(
        Compiler& cc, const bytecode::BytecodeProgram& program,
        size_t startOffset, size_t endOffset,
        size_t rangeStart, size_t rangeEnd)
    {
        std::unordered_map<size_t, Label> labels;
        for (size_t ip = startOffset; ip < endOffset; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (instr.opcode == OpCode::JUMP || instr.opcode == OpCode::JUMP_IF_FALSE ||
                instr.opcode == OpCode::JUMP_IF_TRUE ||
                instr.opcode == OpCode::JUMP_IF_FALSE_OR_POP ||
                instr.opcode == OpCode::JUMP_IF_TRUE_OR_POP ||
                instr.opcode == OpCode::JUMP_BACK)
            {
                if (!instr.operands.empty())
                {
                    size_t target = instr.operands[0];
                    if (target >= rangeStart && target <= rangeEnd &&
                        labels.find(target) == labels.end())
                    {
                        labels[target] = cc.new_label();
                    }
                }
            }
        }
        return labels;
    }

    std::unordered_set<size_t> collectBackEdgeTargets(
        const bytecode::BytecodeProgram& program,
        size_t startOffset, size_t endOffset)
    {
        std::unordered_set<size_t> targets;
        for (size_t ip = startOffset; ip < endOffset; ++ip)
        {
            const auto& instr = program.getInstruction(ip);
            if (instr.opcode == OpCode::JUMP_BACK && !instr.operands.empty())
            {
                targets.insert(instr.operands[0]);
            }
        }
        return targets;
    }

    void emitMemoryInit(Compiler& cc,
                        Gp localsBase, size_t localCount,
                        Gp boxedBase, size_t boxedCount)
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
        cc.mov(bsCount, static_cast<int64_t>(boxedCount));
        InvokeNode* initBoxed;
        cc.invoke(Out(initBoxed), reinterpret_cast<uint64_t>(jit_locals_init),
                  FuncSignature::build<void, value::Value*, size_t>());
        initBoxed->set_arg(0, bsBase);
        initBoxed->set_arg(1, bsCount);
    }

    // MYT-163 Phase F-a inline-emission helpers.
    //
    // MYT-169 (Phase F-a follow-up): emit the receiver ClassDef* extract
    // inline. Replaces the jit_extract_classdef helper call in both MONO
    // (emitInlineShapeGuard) and POLY (emitExtractReceiverClassDef) paths,
    // saving ~30 cyc/call on the hot inline-method path.
    //
    // Emitted sequence:
    //   tag := (byte)[receiverAddr + variantTagOffset]
    //   if (tag == valueObjAltTag) goto lblValueObj
    //   if (tag != objInstAltTag)  goto slowLabel
    //   rawPtr := [receiverAddr + 0]                        ; shared_ptr._Ptr
    //   if (rawPtr == 0) goto slowLabel
    //   classDef := [rawPtr + objInstClassDefOffset + 0]    ; ClassDef* via shared_ptr._Ptr
    //   goto lblMerge
    // lblValueObj:
    //   rawPtr := [receiverAddr + 0]
    //   if (rawPtr == 0) goto slowLabel
    //   classDef := [rawPtr + valueObjClassDefOffset + 0]
    // lblMerge:
    static Gp emitInlineExtractClassDef(JitEmissionState& s, int receiverStackIdx,
                                        asmjit::Label slowLabel)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        const auto& layout = getInlineShapeLayout();

        Gp receiverAddr = cc.new_gp64();
        cc.lea(receiverAddr, Mem(s.boxedBase,
                                  static_cast<int32_t>(receiverStackIdx * valueSize)));

        Gp tag = cc.new_gp32();
        cc.movzx(tag, byte_ptr(receiverAddr,
                                static_cast<int32_t>(layout.variantTagOffset)));

        Label lblValueObj = cc.new_label();
        Label lblMerge    = cc.new_label();

        // Dispatch by receiver kind; ObjectInstance is the hot fall-through.
        cc.cmp(tag, static_cast<int32_t>(layout.valueObjAltTag));
        cc.je(lblValueObj);
        cc.cmp(tag, static_cast<int32_t>(layout.objInstAltTag));
        cc.jne(slowLabel);

        Gp rawPtr   = cc.new_gp64();
        Gp classDef = cc.new_gp64();

        cc.mov(rawPtr, qword_ptr(receiverAddr, 0));
        cc.test(rawPtr, rawPtr);
        cc.jz(slowLabel);
        cc.mov(classDef, qword_ptr(rawPtr,
                                    static_cast<int32_t>(layout.objInstClassDefOffset)));
        cc.jmp(lblMerge);

        cc.bind(lblValueObj);
        cc.mov(rawPtr, qword_ptr(receiverAddr, 0));
        cc.test(rawPtr, rawPtr);
        cc.jz(slowLabel);
        cc.mov(classDef, qword_ptr(rawPtr,
                                    static_cast<int32_t>(layout.valueObjClassDefOffset)));

        cc.bind(lblMerge);
        return classDef;
    }

    void emitInlineShapeGuard(JitEmissionState& s, int receiverStackIdx,
                              const void* expectedShape, asmjit::Label slowLabel)
    {
        auto& cc = s.cc;
        Gp actualShape = emitInlineExtractClassDef(s, receiverStackIdx, slowLabel);

        Gp expectedReg = cc.new_gp64();
        cc.mov(expectedReg, reinterpret_cast<uint64_t>(expectedShape));
        cc.cmp(actualShape, expectedReg);
        cc.jne(slowLabel);
    }

    // MYT-165 (Phase F-c): extract the receiver's ClassDefinition pointer once
    // and return it in a reusable Gp register. The POLY emitter calls this
    // before the guard chain so the N shape-compares all share a single
    // extract instead of N. MYT-169: inlined extract, no helper call.
    //
    // Old helper returned nullptr on non-ObjectInstance / non-ValueObject
    // receivers and the subsequent cmp mismatched; we preserve that contract
    // here by steering the variant/null failure paths to a local "set classDef
    // to 0" block so the POLY guard chain's N immediate-compares still
    // mismatch naturally and fall through to its slowLabel.
    Gp emitExtractReceiverClassDef(JitEmissionState& s, int receiverStackIdx)
    {
        auto& cc = s.cc;
        Label lblBad  = cc.new_label();
        Label lblDone = cc.new_label();
        Gp classDef = emitInlineExtractClassDef(s, receiverStackIdx, lblBad);
        cc.jmp(lblDone);
        cc.bind(lblBad);
        cc.xor_(classDef, classDef);  // classDef := 0 → all compares mismatch
        cc.bind(lblDone);
        return classDef;
    }

    // MYT-165 (Phase F-c): emit a single cmp/jne against the pre-extracted
    // classDef register. Used for each link of the POLY guard chain; the last
    // link's missLabel is the slow-path helper call, intermediate links jump
    // to the next-shape check label.
    void emitInlineShapeGuardReusingClassDef(JitEmissionState& s,
                                              Gp classDefReg,
                                              const void* expectedShape,
                                              asmjit::Label missLabel)
    {
        auto& cc = s.cc;
        Gp expectedReg = cc.new_gp64();
        cc.mov(expectedReg, reinterpret_cast<uint64_t>(expectedShape));
        cc.cmp(classDefReg, expectedReg);
        cc.jne(missLabel);
    }

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
                             const bytecode::BytecodeProgram::FunctionMetadata& callee)
    {
        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;
        static_assert(valueSize % 8 == 0,
                      "Fix B raw-memcpy assumes sizeof(Value) is 8-byte aligned");
        constexpr size_t numQWords = valueSize / 8;

        const size_t total = callee.parameterCount;  // includes implicit `this`
        for (size_t i = 0; i < total; ++i)
        {
            // parameterTypes[0] is `this`'s class name for instance methods
            // — classified as BOXED by the default below. Subsequent entries
            // are user param types, where "int"/"float"/"bool" trigger the
            // fast unbox/rebox path. Mirrors emitArgumentUnboxing in Core.
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

            Gp src = cc.new_gp64();
            cc.lea(src, Mem(s.boxedBase,
                            static_cast<int32_t>((receiverStackIdx + static_cast<int>(i)) * valueSize)));
            Gp dst = cc.new_gp64();
            cc.lea(dst, Mem(s.localsBase,
                            static_cast<int32_t>((localsBaseSlot + i) * s.localStride)));

            // MYT-169 Fix B (boxed) + Fix C (primitive): unified raw byte copy.
            // For ALL param types, memcpy the Value bytes from the caller's
            // operand-stack slot into the callee's local slot. The bytecode
            // compiler guarantees src already holds a Value of the declared
            // parameter type (int64_t / double / bool / shared_ptr<...>), so
            // the local ends up with a byte-identical Value — LOAD_LOCAL in
            // the callee body reads it via jit_unbox_* (for primitives) or
            // jit_value_copy (for boxed) exactly as before.
            //
            // For BOXED params (Fix B): also reset the source slot's variant
            // tag to monostate so its destructor on subsequent overwrites is a
            // no-op — we have donated ownership to the local slot. The local's
            // matching ~shared_ptr runs at emitInlineLocalDestroy (end of the
            // inline body). Saves the ~atomic refcount bump of jit_value_copy.
            //
            // For primitive params (Fix C): no tag reset / destroy needed —
            // int64_t / double / bool Value alternatives have trivial dtors.
            const auto& layout = getInlineShapeLayout();
            Gp scratch = cc.new_gp64();
            for (size_t k = 0; k < numQWords; ++k)
            {
                cc.mov(scratch, qword_ptr(src, static_cast<int32_t>(k * 8)));
                cc.mov(qword_ptr(dst, static_cast<int32_t>(k * 8)), scratch);
            }
            if (isBoxedSlotType(paramType))
            {
                cc.mov(byte_ptr(src, static_cast<int32_t>(layout.variantTagOffset)),
                        static_cast<int32_t>(layout.monostateAltTag));
            }

            s.localTypes[localsBaseSlot + i] = paramType;
        }
    }

    // MYT-169 Fix B: release the donated shared_ptr ownership that
    // emitInlineLocalCopy's raw memcpy left aliased in each boxed inline-local
    // slot. Primitive params (INT/FLOAT/BOOL) have trivial Value destructors
    // and are skipped. jit_value_destroy leaves the slot as monostate so the
    // next inline-call iteration's memcpy overwrites safely, and the final
    // emitCleanup at function exit sees a monostate (no-op ~Value).
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
