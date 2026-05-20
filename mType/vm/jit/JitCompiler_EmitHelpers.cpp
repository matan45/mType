#include "JitEmissionState.hpp"
#include <asmjit/x86.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include "JitCodeCache.hpp"
#include "JitHelpers.hpp"
#include "../bytecode/OpCode.hpp"
#include "../optimization/InlineAnalysis.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../value/ValueBridge.hpp"
#include "../../value/ValueObject.hpp"

namespace vm::jit
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using OpCode = bytecode::OpCode;

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

    // MYT-316: cheap eligibility pre-check used by the scan below to decide
    // whether a plain CALL / CALL_FAST should trigger boxed-mode emission.
    // The scan runs once per JIT/OSR compile, BEFORE emission, so this must
    // not duplicate work the real inliner does at emit time — it only needs
    // to be conservative enough that real misses (self-recursion, oversized
    // bodies, try/catch) don't flip the enclosing function into boxed mode
    // for no payoff. Full eligibility / safety checks still run in
    // tryEmitInlinedFunctionCall.
    //
    // `selfFnName` is the function being compiled (mangled name preferred,
    // plain name fallback). Empty for OSR — OSR has no static caller
    // identity, so self-recursion can't be excluded at scan time; the real
    // inliner still rejects it via checkFunctionInlineEligibility.
    static bool calleeMightBeInlineable(
        const bytecode::BytecodeProgram& program,
        const bytecode::BytecodeProgram::Instruction& instr,
        bool isCallFast,
        const std::string& selfFnName)
    {
        const bytecode::BytecodeProgram::FunctionMetadata* callee = nullptr;
        if (isCallFast)
        {
            callee = program.getFunctionByIndex(
                static_cast<size_t>(instr.inlineOperands[0]));
        }
        else
        {
            const auto idx = static_cast<uint32_t>(instr.inlineOperands[0]);
            if (idx >= program.getConstantPool().strings.size()) return false;
            const std::string& name = program.getConstantPool().getString(idx);
            callee = program.getFunction(name);
        }
        if (!callee) return false;
        if (callee->isNative || callee->isAsync) return false;
        if (callee->instructionCount == 0) return false;
        if (!callee->exceptionTable.getEntries().empty()) return false;
        if (callee->returnType == "void") return false;

        bool hasNewStack = false;
        const size_t end = callee->startOffset + callee->instructionCount;
        for (size_t cip = callee->startOffset; cip < end; ++cip)
        {
            if (program.getInstruction(cip).opcode == OpCode::NEW_STACK)
            {
                hasNewStack = true;
                break;
            }
        }

        const size_t sizeLimit = hasNewStack
            ? optimization::INLINE_SCOPE_STACK_SIZE_LIMIT
            : optimization::INLINE_SIZE_LIMIT;
        if (callee->instructionCount > sizeLimit) return false;

        // Self-recursion: the real inliner will reject this via SELF_RECURSIVE
        // and tryEmitSelfTailCall / tryEmitSelfDirectCall would have a much
        // better shot in unboxed mode. Don't flip the function to boxed mode
        // for a call site that can't benefit from inlining.
        if (!selfFnName.empty())
        {
            if (callee->name == selfFnName || callee->mangledName == selfFnName)
                return false;
        }

        // Mirror the HAS_NESTED_CALL / HAS_TRY_CATCH / HAS_ASYNC opcodes from
        // scanCalleeOpcodes (InlineAnalysis.cpp). The full deny-list runs at
        // emit time; the pre-check just needs to avoid flipping boxed mode
        // for callees that won't actually inline. Capped at INLINE_SIZE_LIMIT
        // ops above, so the inner loop is O(<=32) per CALL site.
        //
        // MYT-352: NEW_STACK is now an *enabling* opcode, not a blocker —
        // it's the whole point of the lift. Don't deny boxed-mode flipping
        // for callees with NEW_STACK; the real inliner accepts them (per
        // the matching MYT-352 change in InlineAnalysis.cpp).
        for (size_t cip = callee->startOffset; cip < end; ++cip)
        {
            const auto& cinstr = program.getInstruction(cip);
            switch (cinstr.opcode)
            {
                case OpCode::CALL: case OpCode::CALL_FAST:
                case OpCode::CALL_METHOD: case OpCode::CALL_METHOD_CACHED:
                case OpCode::CALL_METHOD_POLY_CACHED:
                case OpCode::CALL_STATIC:
                case OpCode::INVOKE:
                case OpCode::TRY_BEGIN: case OpCode::TRY_END:
                case OpCode::CATCH: case OpCode::FINALLY: case OpCode::THROW:
                case OpCode::AWAIT: case OpCode::CREATE_PROMISE:
                case OpCode::SUPER_INVOKE: case OpCode::SUPER_CONSTRUCTOR:
                case OpCode::SUPER_GET_FIELD: case OpCode::SUPER_SET_FIELD:
                case OpCode::GET_SUPER:
                case OpCode::STRING_BUILD:
                case OpCode::JUMP_IF_NULL:
                    return false;
                default:
                    break;
            }
        }
        return true;
    }

    bool scanOpcodesForBoxedTypes(const bytecode::BytecodeProgram& program,
                                  size_t startOffset, size_t endOffset,
                                  const std::string& selfFnName)
    {
        for (size_t ip = startOffset; ip < endOffset; ++ip)
        {
            const auto& si = program.getInstruction(ip);
            // MYT-316: CALL / CALL_FAST trigger boxed mode only when the
            // callee is a plausible inline candidate. Self-recursive
            // functions (fib/ack/gcd) keep their unboxed-mode TCO and
            // direct-call optimizations.
            if (si.opcode == OpCode::CALL || si.opcode == OpCode::CALL_FAST)
            {
                if (calleeMightBeInlineable(program, si,
                        si.opcode == OpCode::CALL_FAST, selfFnName))
                    return true;
                continue;
            }
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
                case OpCode::ARRAY_GET_ALIAS:
                case OpCode::ARRAY_SET:   case OpCode::ARRAY_LENGTH:
                case OpCode::ARRAY_GET_INT_LOCAL: case OpCode::ARRAY_SET_INT_LOCAL:
                case OpCode::ARRAY_LENGTH_LOCAL:
                case OpCode::INSTANCEOF:  case OpCode::INSTANCEOF_TYPEPARAM:
                case OpCode::CAST:        case OpCode::CAST_TYPEPARAM:
                // MYT-147: iterator opcodes receive / produce boxed
                // ObjectInstance values on the operand stack, so the
                // enclosing function must run in boxed-types mode.
                case OpCode::GET_ITERATOR:      case OpCode::ITERATOR_HAS_NEXT:
                case OpCode::ITERATOR_NEXT:     case OpCode::ITERATOR_CLOSE:
                // Specialized primitive-method opcodes receive boxed
                // Int / Float objects, so the enclosing function emits in
                // boxed-types mode.
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
                // Bool/String object methods receive boxed Bool/String objects.
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
            if (si.opcode == OpCode::CALL && si.numOperands() >= 2)
            {
                uint32_t fnIdx = static_cast<uint32_t>(si.inlineOperands[0]);
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
                if (instr.hasOperands())
                {
                    size_t target = instr.inlineOperands[0];
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
            if (instr.opcode == OpCode::JUMP_BACK && instr.hasOperands())
            {
                targets.insert(instr.inlineOperands[0]);
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
    // (MSVC shared_ptr._Ptr) is the raw T*.
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
}
