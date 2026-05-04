#include "JitEmissionState.hpp"
#include <cstddef>
#include "JitHelpers.hpp"
#include "JitCodeCache.hpp"
#include "../bytecode/OpCode.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../value/ValueObject.hpp"
#include "../../value/ValueBridge.hpp"
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
        // Layout constants for the donation-reset in emitInlineLocalCopy. Under
        // the 16-byte tagged Value (MYT-126), tag_ is the first member of
        // value::Value so the tag byte sits at offset 0. Writing ValueType::VOID
        // into that byte donates the slot without a refcount bump — isHeapTag
        // is false for VOID, so the matching emitInlineLocalDestroy skips the
        // release.
        struct InlineShapeLayout
        {
            size_t  tagOffset;
            uint8_t voidTag;
        };
        static_assert(sizeof(value::ValueType) == 1,
                      "tag byte layout assumes single-byte ValueType");

        const InlineShapeLayout& getInlineShapeLayout()
        {
            static constexpr InlineShapeLayout layout{
                /*tagOffset=*/0,
                static_cast<uint8_t>(value::ValueType::VOID)
            };
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
                case OpCode::GET_FIELD_CACHED: case OpCode::SET_FIELD_CACHED:
                case OpCode::SET_FIELD:   case OpCode::INLINE_SET_FIELD:
                case OpCode::INLINE_GET_FIELD:
                // MYT-152: LOAD_VAR / STORE_VAR produce / consume boxed Values
                // (global or field lookups have no compile-time primitive
                // type), so the enclosing loop must emit in boxed mode.
                // MYT-204: _CACHED variants follow the same boxed-mode rule.
                case OpCode::LOAD_VAR:    case OpCode::STORE_VAR:
                case OpCode::LOAD_VAR_CACHED: case OpCode::STORE_VAR_CACHED:
                case OpCode::NEW_OBJECT:
                case OpCode::NEW_STACK:   // MYT-134
                case OpCode::NEW_VALUE_OBJECT: case OpCode::OBJECT_TO_VALUE:
                case OpCode::CREATE_PROMISE:
                case OpCode::OBJECT_TO_VALUE_CREATE_PROMISE:
                case OpCode::CREATE_PROMISE_RETURN_VALUE:
                case OpCode::AWAIT:
                case OpCode::CALL_METHOD: case OpCode::CALL_METHOD_CACHED:
                case OpCode::CALL_METHOD_POLY_CACHED:  // MYT-203
                case OpCode::CALL_STATIC:
                // MYT-198 / MYT-203: fused _CACHED ops participate in boxed-
                // types mode for the same reason their un-fused variants do —
                // they produce / consume boxed Values on the operand stack.
                case OpCode::LOAD_LOCAL_CALL_CACHED:
                case OpCode::LOAD_LOCAL_CALL_POLY_CACHED:  // MYT-203
                case OpCode::LOAD_LOCAL_GET_FIELD_CACHED:
                case OpCode::NEW_ARRAY:   case OpCode::NEW_ARRAY_MULTI:
                case OpCode::ARRAY_GET:
                case OpCode::ARRAY_SET:   case OpCode::ARRAY_LENGTH:
                case OpCode::ARRAY_GET_INT_LOCAL: case OpCode::ARRAY_SET_INT_LOCAL:
                case OpCode::ARRAY_LENGTH_LOCAL:
                case OpCode::INSTANCEOF:  case OpCode::INSTANCEOF_TYPEPARAM:
                case OpCode::CAST:        case OpCode::CAST_TYPEPARAM:
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
                // Bool/String object methods receive boxed Bool/String objects on
                // the operand stack, same boxed-mode trigger as INVOKE_INT/FLOAT.
                case OpCode::INVOKE_BOOL_AND: case OpCode::INVOKE_BOOL_OR:
                case OpCode::INVOKE_BOOL_XOR: case OpCode::INVOKE_BOOL_NOT:
                case OpCode::INVOKE_BOOL_EQUALS:
                case OpCode::INVOKE_STRING_LENGTH: case OpCode::INVOKE_STRING_CONCAT:
                case OpCode::INVOKE_STRING_EQUALS: case OpCode::INVOKE_STRING_IS_EMPTY:
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
    // MYT-190 Phase 2 (re-inlines the MYT-169 extract under tagged Value).
    // The receiver's shared_ptr<ObjectInstance> / shared_ptr<ValueObject>
    // lives two indirections below the Value: payload_.ptr points to a
    // TypedBridge whose held_ member is the shared_ptr, whose first qword
    // (MSVC shared_ptr._Ptr) is the raw T*. Emitted sequence:
    //
    //   bridge := [rcv + Value::payloadOffset()]
    //   if (bridge == 0) goto slowLabel
    //   tag := (byte)[rcv + 0]
    //   if (tag == VALUE_OBJECT) goto lblValueObj
    //   if (tag != OBJECT) goto slowLabel
    //   rawPtr := [bridge + ObjBridge::heldOffset()]       ; shared_ptr._Ptr
    //   if (rawPtr == 0) goto slowLabel
    //   classDef := [rawPtr + ObjectInstance::classDefinitionMemberOffset()]
    //   goto lblMerge
    // lblValueObj:
    //   rawPtr := [bridge + ValBridge::heldOffset()]
    //   if (rawPtr == 0) goto slowLabel
    //   classDef := [rawPtr + ValueObject::classDefinitionMemberOffset()]
    // lblMerge:
    //
    // All offsets are queried from the owning C++ types (not baked into
    // emitted code), so layout changes remain a single source-of-truth in
    // ValueBridge.hpp / ValueType.hpp.
    static Gp emitInlineExtractClassDef(JitEmissionState& s, int receiverStackIdx,
                                        asmjit::Label slowLabel)
    {
        using ObjBridge = value::TypedBridge<
            value::BridgeKind::OBJECT_INSTANCE,
            std::shared_ptr<runtimeTypes::klass::ObjectInstance>>;
        using ValBridge = value::TypedBridge<
            value::BridgeKind::VALUE_OBJECT,
            std::shared_ptr<value::ValueObject>>;

        auto& cc = s.cc;
        constexpr size_t valueSize = JitEmissionState::VALUE_SIZE;

        Gp receiverAddr = cc.new_gp64();
        cc.lea(receiverAddr, Mem(s.boxedBase,
                                  static_cast<int32_t>(receiverStackIdx * valueSize)));

        Gp bridge = cc.new_gp64();
        cc.mov(bridge, qword_ptr(receiverAddr,
                                 static_cast<int32_t>(value::Value::payloadOffset())));
        cc.test(bridge, bridge);
        cc.jz(slowLabel);

        Gp tag = cc.new_gp32();
        cc.movzx(tag, byte_ptr(receiverAddr, 0));

        Label lblValueObj = cc.new_label();
        // MYT-208: STACK_OBJECT branch — `bridge` field actually holds a raw
        // ObjectInstance* (the union's stackObject member is at the payload
        // offset), so skip the heldOffset indirection and read ClassDef
        // directly. ClassDef offset on ObjectInstance is identical regardless
        // of how we got the raw pointer.
        Label lblStackObj = cc.new_label();
        Label lblMerge    = cc.new_label();

        // Dispatch by receiver kind; ObjectInstance is the hot fall-through.
        cc.cmp(tag, static_cast<int32_t>(value::ValueType::VALUE_OBJECT));
        cc.je(lblValueObj);
        cc.cmp(tag, static_cast<int32_t>(value::ValueType::STACK_OBJECT));
        cc.je(lblStackObj);
        cc.cmp(tag, static_cast<int32_t>(value::ValueType::OBJECT));
        cc.jne(slowLabel);

        Gp rawPtr   = cc.new_gp64();
        Gp classDef = cc.new_gp64();

        cc.mov(rawPtr, qword_ptr(bridge,
                                 static_cast<int32_t>(ObjBridge::heldOffset())));
        cc.test(rawPtr, rawPtr);
        cc.jz(slowLabel);
        cc.mov(classDef, qword_ptr(rawPtr,
                                    static_cast<int32_t>(
                                        runtimeTypes::klass::ObjectInstance::classDefinitionMemberOffset())));
        cc.jmp(lblMerge);

        cc.bind(lblValueObj);
        cc.mov(rawPtr, qword_ptr(bridge,
                                 static_cast<int32_t>(ValBridge::heldOffset())));
        cc.test(rawPtr, rawPtr);
        cc.jz(slowLabel);
        cc.mov(classDef, qword_ptr(rawPtr,
                                    static_cast<int32_t>(
                                        value::ValueObject::classDefinitionMemberOffset())));
        cc.jmp(lblMerge);

        // STACK_OBJECT: bridge already IS the raw ObjectInstance* — `bridge`
        // and `payload.stackObject` share the same union offset.
        cc.bind(lblStackObj);
        cc.mov(classDef, qword_ptr(bridge,
                                    static_cast<int32_t>(
                                        runtimeTypes::klass::ObjectInstance::classDefinitionMemberOffset())));

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
            // slotType (bytecode-compiler invariant), but the previous
            // implementation raw-memcpy'd from boxedBase unconditionally,
            // leaving primitive locals filled with stale boxedBase bytes ->
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
                // BOXED param: raw memcpy from boxedBase (as before) + donate
                // ownership by resetting source variant tag to monostate. The
                // matching destroy runs at emitInlineLocalDestroy.
                Gp src = cc.new_gp64();
                cc.lea(src, Mem(s.boxedBase,
                                static_cast<int32_t>(srcSlot * valueSize)));
                const auto& layout = getInlineShapeLayout();
                Gp scratch = cc.new_gp64();
                for (size_t k = 0; k < numQWords; ++k)
                {
                    cc.mov(scratch, qword_ptr(src, static_cast<int32_t>(k * 8)));
                    cc.mov(qword_ptr(dst, static_cast<int32_t>(k * 8)), scratch);
                }
                cc.mov(byte_ptr(src, static_cast<int32_t>(layout.tagOffset)),
                        static_cast<int32_t>(layout.voidTag));
            }
            else
            {
                // Primitive param: box the caller's stackBase raw primitive
                // into the callee's local slot as a valid Value. LOAD_LOCAL
                // in the callee body reads this via jit_unbox_* and gets the
                // correct primitive. No destroy needed (trivial dtor).
                // Also reset the stackBase source mirror to zero for hygiene
                // is NOT required — stackBase slots are raw, not Values, and
                // the caller's subsequent reuse of the slot overwrites.
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

    // MYT-185/186/187: physical-state fix. The slow path's emitReturnValueCopyBoxed
    // populates BOTH boxedBase[receiverStackIdx] (via jit_value_copy from
    // ctx->returnValue) AND stackBase[receiverStackIdx] (via jit_unbox_int
    // mirror), and pushes slotTypes BOXED. Compile-time state at endLabel
    // therefore expects top = BOXED with both slots valid. The fast path's
    // callee body in boxed-mode emission leaves the return value in exactly
    // one of the two: INT/BOOL primitives live in stackBase (emitLoadLocal
    // unboxes into stackBase; ADD_INT / arithmetic produce INT in stackBase);
    // BOXED family values (GET_FIELD results, string concats, CALL_METHOD
    // returns) live in boxedBase. This helper converges both slots so the
    // endLabel join sees matching physical state regardless of which path
    // ran.
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
