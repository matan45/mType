#include "JitHelpers.hpp"
#include "JitCodeCache.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "ic/InlineCacheTable.hpp"
#include "../../value/ValueShim.hpp"
#include "../../errors/AccessViolationException.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../runtime/utils/NullCheckUtils.hpp"
#include "../runtime/utils/MethodResolver.hpp"
#include "../runtime/utils/TypeArgResolution.hpp"
#include "../bytecode/OpCode.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
#include "../../environment/Environment.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/klass/SignatureUtils.hpp"
#include "../../value/ObjectInstancePool.hpp"
#include "../../value/ValueObject.hpp"
#include "../../value/PrimitiveTypeTag.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace vm::jit
{
    static value::PrimitiveTypeTag getPrimitiveProtocolTag(const value::Value& v) noexcept
    {
        if (value::isValueObject(v))
        {
            const auto& valueObj = value::asValueObject(v);
            return valueObj ? valueObj->getPrimitiveTag() : value::PrimitiveTypeTag::NONE;
        }
        if (value::isAnyObject(v))
        {
            auto* raw = value::asObjectInstanceRaw(v);
            if (!raw || !raw->getClassDefinition()) return value::PrimitiveTypeTag::NONE;
            return value::classNameToPrimitiveTag(raw->getClassDefinition()->getName());
        }
        return value::PrimitiveTypeTag::NONE;
    }

    static const value::Value* getPrimitiveProtocolField0(const value::Value& v) noexcept
    {
        if (value::isValueObject(v))
        {
            const auto& valueObj = value::asValueObject(v);
            if (!valueObj || valueObj->getFieldCount() == 0) return nullptr;
            return &valueObj->getFieldByIndex(0);
        }
        if (value::isAnyObject(v))
        {
            auto* raw = value::asObjectInstanceRaw(v);
            if (!raw || !raw->hasFieldVector()) return nullptr;
            return &raw->getFieldByIndex(0);
        }
        return nullptr;
    }

    static bool computePrimitiveProtocolHash(value::Value& out, const value::Value& receiver)
    {
        const value::PrimitiveTypeTag tag = getPrimitiveProtocolTag(receiver);
        if (tag == value::PrimitiveTypeTag::NONE) return false;
        const value::Value* field = getPrimitiveProtocolField0(receiver);
        if (!field) return false;

        switch (tag)
        {
            case value::PrimitiveTypeTag::INT:
                if (!value::isInt(*field)) return false;
                out = static_cast<int64_t>(std::hash<int64_t>{}(value::asInt(*field)) & 0x7FFFFFFF);
                return true;
            case value::PrimitiveTypeTag::FLOAT:
                if (!value::isFloat(*field)) return false;
                out = static_cast<int64_t>(std::hash<double>{}(value::asFloat(*field)) & 0x7FFFFFFF);
                return true;
            case value::PrimitiveTypeTag::BOOL:
                if (!value::isBool(*field)) return false;
                out = value::asBool(*field) ? static_cast<int64_t>(1231) : static_cast<int64_t>(1237);
                return true;
            case value::PrimitiveTypeTag::STRING:
                if (value::isString(*field))
                {
                    out = static_cast<int64_t>(std::hash<std::string>{}(value::asString(*field)) & 0x7FFFFFFF);
                    return true;
                }
                if (value::isInternedString(*field))
                {
                    out = static_cast<int64_t>(
                        std::hash<std::string>{}(value::asInternedString(*field).getString()) & 0x7FFFFFFF);
                    return true;
                }
                return false;
            case value::PrimitiveTypeTag::NONE:
                return false;
        }
        return false;
    }

    static bool computePrimitiveProtocolEquals(value::Value& out,
                                               const value::Value& receiver,
                                               const value::Value& arg)
    {
        const value::PrimitiveTypeTag tag = getPrimitiveProtocolTag(receiver);
        if (tag == value::PrimitiveTypeTag::NONE || getPrimitiveProtocolTag(arg) != tag)
            return false;

        const value::Value* left = getPrimitiveProtocolField0(receiver);
        const value::Value* right = getPrimitiveProtocolField0(arg);
        if (!left || !right) return false;

        switch (tag)
        {
            case value::PrimitiveTypeTag::INT:
                if (!value::isInt(*left) || !value::isInt(*right)) return false;
                out = value::asInt(*left) == value::asInt(*right);
                return true;
            case value::PrimitiveTypeTag::FLOAT:
                if (!value::isFloat(*left) || !value::isFloat(*right)) return false;
                out = value::asFloat(*left) == value::asFloat(*right);
                return true;
            case value::PrimitiveTypeTag::BOOL:
                if (!value::isBool(*left) || !value::isBool(*right)) return false;
                out = value::asBool(*left) == value::asBool(*right);
                return true;
            case value::PrimitiveTypeTag::STRING:
                if (!(value::isString(*left) || value::isInternedString(*left)) ||
                    !(value::isString(*right) || value::isInternedString(*right)))
                    return false;
                out = *left == *right;
                return true;
            case value::PrimitiveTypeTag::NONE:
                return false;
        }
        return false;
    }

    static bool tryPrimitiveProtocolFastCall(value::Value& out,
                                             ic::MethodProtocolFastKind kind,
                                             const value::Value& receiver,
                                             const value::Value* args,
                                             size_t argCount)
    {
        switch (kind)
        {
            case ic::MethodProtocolFastKind::PRIMITIVE_HASH_CODE:
                if (argCount != 0) return false;
                return computePrimitiveProtocolHash(out, receiver);
            case ic::MethodProtocolFastKind::PRIMITIVE_EQUALS:
                if (argCount != 1 || !args) return false;
                return computePrimitiveProtocolEquals(out, receiver, args[0]);
            case ic::MethodProtocolFastKind::NONE:
                return false;
        }
        return false;
    }

    static ic::MethodProtocolFastKind classifyPrimitiveProtocolFastKind(
        const runtimeTypes::klass::ClassDefinition* classDef,
        const std::string& simpleMethodName,
        size_t argCount,
        const std::string& definingClassName)
    {
        (void)definingClassName;
        if (!classDef || value::classNameToPrimitiveTag(classDef->getName()) == value::PrimitiveTypeTag::NONE)
            return ic::MethodProtocolFastKind::NONE;
        if (simpleMethodName == "hashCode" && argCount == 0)
            return ic::MethodProtocolFastKind::PRIMITIVE_HASH_CODE;
        if (simpleMethodName == "equals" && argCount == 1)
            return ic::MethodProtocolFastKind::PRIMITIVE_EQUALS;
        return ic::MethodProtocolFastKind::NONE;
    }

    // MYT-163: speculative inlining shape-guard helper. Extracts the raw
    // ClassDefinition pointer from a receiver, handling both ObjectInstance
    // and ValueObject variants (MYT-167 F-e). Returns nullptr for any other
    // variant so the guard compare mismatches and the inlined site falls back
    // to jit_call_method_ic. Keeping the shared_ptr layout behind this helper
    // avoids baking MSVC control-block internals into emitted code.
    const void* jit_extract_classdef(const value::Value* receiver)
    {
        if (!receiver) return nullptr;
        // MYT-208: handle OBJECT and STACK_OBJECT through one raw-unwrap path.
        // ClassDef offset on ObjectInstance is identical for both, so the
        // shape-guard's compare against `expectedShape` works unchanged.
        if (value::isAnyObject(*receiver))
        {
            auto* raw = value::asObjectInstanceRaw(*receiver);
            return raw ? raw->getClassDefinition().get() : nullptr;
        }
        if (value::isValueObject(*receiver))
        {
            const auto& valueObj = value::asValueObject(*receiver);
            return valueObj ? valueObj->getClassDefinition().get() : nullptr;
        }
        return nullptr;
    }

    const value::Value* jit_field_data_const(const value::Value* receiver) noexcept
    {
        if (!receiver) return nullptr;
        // MYT-208: accept STACK_OBJECT (raw borrowed) alongside OBJECT.
        if (value::isAnyObject(*receiver))
        {
            auto* raw = value::asObjectInstanceRaw(*receiver);
            if (!raw) return nullptr;
            if (!raw->hasFieldVector()) return nullptr;
            return &raw->getFieldByIndex(0);
        }
        if (value::isValueObject(*receiver))
        {
            const auto& vobj = value::asValueObject(*receiver);
            if (!vobj || vobj->getFieldCount() == 0) return nullptr;
            return &vobj->getFieldByIndex(0);
        }
        return nullptr;
    }

    // MYT-191: inline-IC SET fast path helper. Thin wrapper around
    // ObjectInstance::setFieldByIndex that preserves the full write-barrier
    // sequence (oldPtr refcount decrement, cycle-root mark, fieldValues map
    // sync) while skipping the IC lookup and name resolution that
    // jit_set_field_ic does unconditionally.
    //
    // The emitter gates on the shape cache being MONOMORPHIC and the shape
    // not being a value class, so the receiver is guaranteed to be an
    // ObjectInstance at a matching shape with the cached fieldIndex already
    // resolved. Anything unexpected at runtime (null bridge, wrong variant,
    // uninitialised fieldVector that ensureFieldVector can't recover) signals
    // via a false return so the caller jumps to the original helper-invoke
    // slow path — which handles ValueObject CoW, populates the IC on miss,
    // and runs the same write barrier ObjectInstance::setFieldByIndex runs.
    bool jit_field_set_at(value::Value* destSlot,
                          const value::Value* receiverSlot,
                          size_t fieldIndex,
                          const value::Value* newValue) noexcept
    {
        if (!destSlot || !receiverSlot || !newValue) return false;
        // MYT-208: accept STACK_OBJECT receivers — pool-borrowed lifetime
        // is owned by CallFrame::stackObjects, independent of destSlot, so
        // overwriting destSlot is safe even when destSlot == receiverSlot.
        if (!value::isAnyObject(*receiverSlot)) return false;
        auto* raw = value::asObjectInstanceRaw(*receiverSlot);
        if (!raw) return false;
        // Pin the raw ObjectInstance pointer BEFORE any operation that could
        // overwrite *destSlot. When destSlot == receiverSlot (the common case —
        // emitter passes the same pointer for both), writing through destSlot
        // for an OBJECT receiver releases the shared_ptr that currently holds
        // the only strong ref to this instance, dropping its refcount to zero
        // and freeing the object. Any use of `raw` after the overwrite would
        // be a use-after-free for OBJECT. STACK_OBJECT receivers don't have
        // this concern (lifetime is in stackObjects) but the same sequencing
        // is harmless. Sequencing below: all work that touches `raw` runs
        // first; the slot overwrite is strictly the last action.
        if (!raw->hasFieldVector())
            raw->ensureFieldVector();
        const auto& classDef = raw->getClassDefinition();
        if (!classDef || fieldIndex >= classDef->getTotalFieldCount())
            return false;
        // setFieldByIndex runs the write barrier: releases oldPtr if distinct
        // from newPtr, marks `this` as cycle-suspect if the new value is a
        // heap ref, keeps fieldValues map in sync. Value::operator= inside
        // the vector assignment retains newPtr.
        raw->setFieldByIndex(fieldIndex, *newValue);
        // Last access to the instance completed above. Mirror jit_set_field_ic's
        // `*destValue = *newValue` semantics. When destSlot == receiverSlot this
        // releases the ObjectInstance bridge the slot held and retains newValue
        // in its place. DO NOT insert any code that references `raw` / `instance`
        // after this line — the backing object may be freed.
        *destSlot = *newValue;
        return true;
    }

    void jit_call_method(JitContext* ctx, uint32_t methodNameIndex, size_t argCount)
    {
        if (ctx->pendingException)
            return;

        try
        {
            value::Value& objectValue = ctx->callArgs[0];

            if (value::isLambda(objectValue))
            {
                if (ctx->vm)
                {
                    auto lambda = value::asLambda(objectValue);
                    ctx->returnValue = ctx->vm->invokeLambda(
                        lambda, &ctx->callArgs[1], argCount);
                    ctx->hasReturnValue = true;
                    return;
                }
            }

            const std::string& methodName = ctx->program->getConstantPool().getString(methodNameIndex);

            std::vector<value::Value> args;
            for (size_t i = 1; i <= argCount; i++)
            {
                args.push_back(ctx->callArgs[i]);
            }

            // MYT-208: route STACK_OBJECT receivers through the Value-receiver
            // overload so the VM dispatch tag-branches frame.thisInstance vs
            // thisInstanceRaw correctly. Heap (OBJECT) receivers stay on the
            // shared_ptr fast path.
            if (value::isStackObject(objectValue))
            {
                if (ctx->vm)
                {
                    ctx->returnValue = ctx->vm->callMethodFromJit(objectValue, methodName, args);
                    ctx->hasReturnValue = true;
                    return;
                }
            }

            if (value::isObject(objectValue))
            {
                auto instance = value::asObject(objectValue);

                if (ctx->vm)
                {
                    ctx->returnValue = ctx->vm->callMethodFromJit(instance, methodName, args);
                    ctx->hasReturnValue = true;
                    return;
                }
            }

            if (value::isValueObject(objectValue))
            {
                if (ctx->vm)
                {
                    // Value-class perf fix: delegate to the Value-receiver VM
                    // overload, which batch-materialises the temp ObjectInstance
                    // via ObjectInstance::loadFromValueObject (vector copy +
                    // one hashmap fill) instead of the N × setField hot loop
                    // this branch previously inlined. In-method mutation
                    // semantics preserved — the temp is still independent of
                    // the caller's ValueObject.
                    ctx->returnValue = ctx->vm->callMethodFromJit(objectValue, methodName, args);
                    ctx->hasReturnValue = true;
                    return;
                }
            }

            // MYT-254: LAMBDA receiver. Per-spec lambdas are invoked via
            // CALL_METHOD apply (see lib/functional/Function.mt etc.) — the
            // interpreter's ObjectExecutor::handleCallMethod (ObjectExecutor.cpp:978)
            // routes such receivers to invokeLambdaMethod. Mirror that here:
            // delegate to VirtualMachine::invokeLambda, which sets up the
            // lambda's CallFrame, restores capturedThis / capturedFrame, and
            // drives the body to completion. Without this branch the slow
            // path falls through to the throw below, the exception lands in
            // ctx->pendingException, and every subsequent jit_call_method_ic
            // becomes a silent no-op — turning the OSR'd outer loop into an
            // infinite spin (stream_pipeline_hot regression).
            if (value::isLambda(objectValue))
            {
                if (ctx->vm)
                {
                    auto lambda = value::asLambda(objectValue);
                    ctx->returnValue = ctx->vm->invokeLambda(
                        lambda, &ctx->callArgs[1], argCount);
                    ctx->hasReturnValue = true;
                    return;
                }
            }

            throw errors::RuntimeException("JIT: cannot call method '" + methodName + "'");
        }
        catch (const OSRDeoptException&)
        {
            throw;
        }
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }

    // MYT-184: the direct JIT->JIT method dispatch (tryDirectJitMethodDispatch,
    // originally MYT-132 / MYT-161) was removed after it was found to corrupt the
    // native stack. When invoked nested, the asmjit-compiled callee's stack-frame
    // layout (cc.new_stack for locals/operand/boxed areas, sized with
    // INLINE_LOCALS_SLACK) wrote past its allocated bounds and clobbered the MSVC
    // /GS cookie of a caller on the chain. Every run ended in
    // STATUS_STACK_BUFFER_OVERRUN (0xC0000409) at __report_gsfailure — never an
    // argument-layout mismatch as the original ticket hypothesised. All method IC
    // hits now route through callMethodFromJitDirect's mini-interpret loop, which
    // runs the callee bytecode directly and is immune to this class of bug. A
    // future dedicated "nested entry" prologue (option b in the MYT-184 plan) can
    // restore the fast path once stack-layout invariants are worked out.

    bool jit_try_primitive_protocol_hash(value::Value* out,
                                         const value::Value* receiver)
    {
        if (!out || !receiver) return false;
        return computePrimitiveProtocolHash(*out, *receiver);
    }

    bool jit_try_primitive_protocol_equals(value::Value* out,
                                           const value::Value* receiver,
                                           const value::Value* arg)
    {
        if (!out || !receiver || !arg) return false;
        return computePrimitiveProtocolEquals(*out, *receiver, *arg);
    }

    void jit_call_method_ic(JitContext* ctx,
                             size_t bytecodeOffset,
                             uint32_t methodNameIndex,
                             size_t argCount)
    {
        if (ctx->pendingException)
            return;

        // Without an IC table or VM, fall back to the resolution path.
        if (!ctx->icTable || !ctx->vm)
        {
            jit_call_method(ctx, methodNameIndex, argCount);
            return;
        }

        try
        {
            value::Value& receiverValue = ctx->callArgs[0];

            if (value::isLambda(receiverValue))
            {
                auto lambda = value::asLambda(receiverValue);
                ctx->returnValue = ctx->vm->invokeLambda(
                    lambda, &ctx->callArgs[1], argCount);
                ctx->hasReturnValue = true;
                return;
            }

            // Unified receiver-kind handling: ObjectInstance and ValueObject
            // hits both route through callMethodFromJitDirect(Value). The
            // Value-receiver overload batch-materialises a temp ObjectInstance
            // for value-class receivers (ObjectInstance::loadFromValueObject)
            // and delegates to the shared_ptr path so the callee runs through
            // one mini-interpret loop regardless of receiver kind. The IC
            // entry's pre-resolved funcMetadata avoids the
            // findInstanceMethodInHierarchy + program->getFunction lookups
            // that the miss path in jit_call_method still performs.
            const runtimeTypes::klass::ClassDefinition* classDef = nullptr;
            bool receiverIsValueObject = false;

            // MYT-208: cover OBJECT and STACK_OBJECT through one raw-unwrap
            // path; the IC shape key is ClassDefinition*, identical for both.
            if (value::isAnyObject(receiverValue))
            {
                auto* raw = value::asObjectInstanceRaw(receiverValue);
                if (!raw)
                {
                    jit_call_method(ctx, methodNameIndex, argCount);
                    return;
                }
                classDef = raw->getClassDefinition().get();
            }
            else if (value::isValueObject(receiverValue))
            {
                const auto& valueObj = value::asValueObject(receiverValue);
                if (!valueObj)
                {
                    jit_call_method(ctx, methodNameIndex, argCount);
                    return;
                }
                classDef = valueObj->getClassDefinition().get();
                receiverIsValueObject = true;
            }
            else
            {
                jit_call_method(ctx, methodNameIndex, argCount);
                return;
            }

            ic::MethodInlineCache& cache = ctx->icTable->getMethodIC(bytecodeOffset);

            // Fast path: monomorphic / polymorphic shape match.
            if (cache.state == ic::ICState::MONOMORPHIC || cache.state == ic::ICState::POLYMORPHIC)
            {
                const ic::MethodICEntry* entry = cache.lookup(classDef);
                if (entry && entry->protocolFastKind != ic::MethodProtocolFastKind::NONE)
                {
                    if (tryPrimitiveProtocolFastCall(
                            ctx->returnValue, entry->protocolFastKind,
                            receiverValue, &ctx->callArgs[1], argCount))
                    {
                        ctx->hasReturnValue = true;
                        return;
                    }
                    // Shape matched but the argument/value layout did not
                    // match the protocol leaf. Fall through to the generic
                    // helper so unusual calls preserve existing semantics.
                    jit_call_method(ctx, methodNameIndex, argCount);
                    return;
                }
                if (entry && entry->funcMetadata)
                {
                    // MYT-184: both receiver kinds route through
                    // callMethodFromJitDirect's mini-interpret loop — direct
                    // JIT→JIT dispatch stays disabled to preserve the /GS
                    // stack-cookie invariant. The Value-receiver overload
                    // handles both: ObjectInstance passes through to the
                    // shared_ptr path bit-identically; ValueObject
                    // batch-materialises a temp ObjectInstance via
                    // loadFromValueObject and then takes the same path.
                    // Either way the IC entry's pre-resolved funcMetadata
                    // lets us skip the findInstanceMethodInHierarchy +
                    // program->getFunction lookups that the slow
                    // jit_call_method path redoes on every iteration.
                    auto* funcMeta = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(entry->funcMetadata);
                    std::vector<value::Value> args;
                    args.reserve(argCount);
                    for (size_t i = 1; i <= argCount; ++i)
                    {
                        args.push_back(ctx->callArgs[i]);
                    }
                    ctx->returnValue = ctx->vm->callMethodFromJitDirect(
                        receiverValue, entry->qualifiedName, funcMeta, args,
                        entry->program);
                    ctx->hasReturnValue = true;
                    return;
                }
            }

            // Miss / UNINITIALIZED / MEGAMORPHIC — defer to the generic helper.
            jit_call_method(ctx, methodNameIndex, argCount);
            if (ctx->pendingException)
                return;

            // Populate cache for future iterations (skip MEGAMORPHIC: addEntry will
            // no-op once the slot count is exceeded, but we save a hash hit + lookup).
            if (cache.state != ic::ICState::MEGAMORPHIC)
            {
                const std::string& rawMethodName =
                    ctx->program->getConstantPool().getString(methodNameIndex);
                const std::string simpleMethodName =
                    runtimeTypes::klass::SignatureUtils::extractSimpleName(rawMethodName);
                auto lookupResult =
                    classDef->findInstanceMethodForCallNameCached(rawMethodName, argCount);
                if (lookupResult.method)
                {
                    // MYT-182: resolve across main + loaded library programs
                    // so the IC entry knows which program owns funcMetadata.
                    const std::vector<const vm::bytecode::BytecodeProgram*>* loadedPrograms =
                        ctx->vm ? &ctx->vm->getLoadedPrograms() : nullptr;
                    size_t startIndex = 0;
                    if (ctx->vm && !ctx->vm->getCallStack().empty())
                    {
                        startIndex = ctx->vm->getCallStack().back().programIndex;
                    }
                    auto resolution = vm::runtime::utils::MethodResolver::resolve(
                        lookupResult.qualifiedName,
                        lookupResult.definingClassName,
                        simpleMethodName,
                        ctx->program,
                        loadedPrograms,
                        startIndex);
                    if (resolution.funcMetadata)
                    {
                        ic::MethodICEntry entry;
                        entry.shape = classDef;
                        entry.funcMetadata = resolution.funcMetadata;
                        entry.startOffset = resolution.funcMetadata->startOffset;
                        entry.qualifiedName = resolution.qualifiedName;
                        entry.program = resolution.program;
                        entry.programIndex = resolution.programIndex;
                        // MYT-167 (F-e): flag reflects the observed receiver
                        // kind. InlineAnalysis::scanCalleeOpcodes consumes
                        // this to apply the read-only restriction for
                        // value-class sites.
                        entry.receiverIsValueObject = receiverIsValueObject;
                        entry.protocolFastKind = classifyPrimitiveProtocolFastKind(
                            classDef, simpleMethodName, argCount,
                            lookupResult.definingClassName);
                        // MYT-183: re-fetch cache reference immediately
                        // before the write. Nested CALL_METHODs in
                        // jit_call_method above may have inserted new
                        // entries into methodCaches and rehashed; the
                        // captured reference is not guaranteed pointer-stable
                        // across that. Cheap on the slow path.
                        ic::MethodInlineCache& freshCache =
                            ctx->icTable->getMethodIC(bytecodeOffset);
                        freshCache.addEntry(entry);
                    }
                }
            }
        }
        catch (const OSRDeoptException&)
        {
            throw;
        }
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }

    int64_t jit_instanceof(const value::Value* val,
                            const vm::bytecode::BytecodeProgram* prog,
                            uint32_t typeIndex)
    {
        const std::string& typeName = prog->getConstantPool().getString(typeIndex);

        if (typeName == "Int" || typeName == "int")
            return value::isInt(*val) ? 1 : 0;
        if (typeName == "Float" || typeName == "float")
            return value::isFloat(*val) ? 1 : 0;
        if (typeName == "Bool" || typeName == "bool")
            return value::isBool(*val) ? 1 : 0;
        if (typeName == "String" || typeName == "string")
            return (value::isString(*val) ||
                    value::isInternedString(*val)) ? 1 : 0;

        // MYT-208: accept STACK_OBJECT alongside OBJECT (instanceof helper).
        if (value::isAnyObject(*val))
        {
            auto* raw = value::asObjectInstanceRaw(*val);
            auto classDef = raw->getClassDefinition();
            while (classDef)
            {
                if (classDef->getName() == typeName) return 1;
                classDef = classDef->getParentClass();
            }
        }

        if (value::isValueObject(*val))
        {
            auto valueObj = value::asValueObject(*val);
            auto classDef = valueObj->getClassDefinition();
            while (classDef)
            {
                if (classDef->getName() == typeName) return 1;
                classDef = classDef->getParentClass();
            }
        }

        return 0;
    }

    void jit_cast(value::Value* dest, const value::Value* src,
                   const vm::bytecode::BytecodeProgram* prog,
                   uint32_t typeIndex)
    {
        const std::string& targetType = prog->getConstantPool().getString(typeIndex);

        if (targetType == "Int" || targetType == "int")
        {
            if (value::isInt(*src))
                { *dest = *src; return; }
            if (value::isFloat(*src))
                { *dest = static_cast<int64_t>(value::asFloat(*src)); return; }
            if (value::isBool(*src))
                { *dest = value::asBool(*src) ? static_cast<int64_t>(1) : static_cast<int64_t>(0); return; }
        }
        else if (targetType == "Float" || targetType == "float")
        {
            if (value::isFloat(*src))
                { *dest = *src; return; }
            if (value::isInt(*src))
                { *dest = static_cast<double>(value::asInt(*src)); return; }
        }
        else if (targetType == "Bool" || targetType == "bool")
        {
            if (value::isBool(*src))
                { *dest = *src; return; }
            if (value::isInt(*src))
                { *dest = (value::asInt(*src) != 0); return; }
        }

        *dest = *src;
    }

    int64_t jit_instanceof_typeparam(const value::Value* val,
                                      JitContext* ctx,
                                      uint32_t paramNameIndex)
    {
        // MYT-228: resolve T against the current call stack via the shared
        // per-frame helper, then delegate to the same name-based instanceof
        // check jit_instanceof uses. Without this resolution the JIT path
        // would try to match the literal name "T" against class hierarchies
        // and always return 0.
        const std::string& paramName = ctx->program->getConstantPool().getString(paramNameIndex);
        const std::string* resolvedPtr = &paramName;

        if (ctx->vm)
        {
            const auto& callStack = ctx->vm->getCallStack();
            for (auto it = callStack.rbegin(); it != callStack.rend(); ++it) {
                if (const auto* p = vm::runtime::utils::resolveTypeParamInFrame(*it, paramName)) {
                    resolvedPtr = p;
                    break;
                }
            }
        }

        const std::string& resolved = *resolvedPtr;

        if (resolved == "Int" || resolved == "int")
            return value::isInt(*val) ? 1 : 0;
        if (resolved == "Float" || resolved == "float")
            return value::isFloat(*val) ? 1 : 0;
        if (resolved == "Bool" || resolved == "bool")
            return value::isBool(*val) ? 1 : 0;
        if (resolved == "String" || resolved == "string")
            return (value::isString(*val) || value::isInternedString(*val)) ? 1 : 0;

        if (value::isAnyObject(*val))
        {
            auto* raw = value::asObjectInstanceRaw(*val);
            auto classDef = raw->getClassDefinition();
            while (classDef)
            {
                if (classDef->getName() == resolved) return 1;
                classDef = classDef->getParentClass();
            }
        }

        if (value::isValueObject(*val))
        {
            auto valueObj = value::asValueObject(*val);
            auto classDef = valueObj->getClassDefinition();
            while (classDef)
            {
                if (classDef->getName() == resolved) return 1;
                classDef = classDef->getParentClass();
            }
        }

        return 0;
    }

    void jit_bind_type_args(JitContext* ctx, uint64_t ip)
    {
        // MYT-228: stage type-arg bindings for the next CALL_*. Populates
        // the pool-backed pending map directly in-place to skip the
        // local-map-then-move dance that setPendingTypeArgs would cost.
        // Mirrors TypeExecutor::handleBindTypeArgs (and shares the
        // per-frame resolve walk via utils::resolveTypeParamInFrame).
        if (!ctx->vm || !ctx->program) return;

        // Per-IP fast path mirroring TypeExecutor::handleBindTypeArgs: when
        // every binding at this site is Concrete, the resolved pairs are
        // stable. Bulk-copy from the cached snapshot and skip the per-binding
        // const-pool indexing + kind branch.
        const size_t bindIp = static_cast<size_t>(ip);
        if (auto* cached = ctx->program->findCachedState(bindIp);
            cached && cached->cachedTypeArgBindingsValid)
        {
            auto& staged = ctx->vm->beginPendingTypeArgs();
            for (const auto& [paramName, resolved] : cached->cachedTypeArgBindings) {
                staged.emplace(paramName, resolved);
            }
            return;
        }

        const auto& instr = ctx->program->getInstructions()[bindIp];
        if (instr.operands.empty()) return;
        const auto& constantPool = ctx->program->getConstantPool();
        const size_t n = static_cast<size_t>(instr.operands[0]);
        if (instr.operands.size() < 1 + 3 * n) return;

        auto& staged = ctx->vm->beginPendingTypeArgs();
        const auto& callStack = ctx->vm->getCallStack();

        bool allConcrete = true;
        std::vector<std::pair<std::string, std::string>> snapshot;
        snapshot.reserve(n);

        for (size_t i = 0; i < n; ++i) {
            const size_t base = 1 + 3 * i;
            const std::string& paramName = constantPool.getString(
                static_cast<uint32_t>(instr.operands[base + 0]));
            const auto kind = static_cast<vm::bytecode::TypeArgValueKind>(
                static_cast<uint8_t>(instr.operands[base + 1]));
            const std::string& rawValue = constantPool.getString(
                static_cast<uint32_t>(instr.operands[base + 2]));

            std::string resolved;
            if (kind == vm::bytecode::TypeArgValueKind::ForwardFromCaller) {
                allConcrete = false;
                if (!callStack.empty()) {
                    if (const auto* p = vm::runtime::utils::resolveTypeParamInFrame(
                            callStack.back(), rawValue)) {
                        resolved = *p;
                    } else {
                        resolved = rawValue;
                    }
                } else {
                    resolved = rawValue;
                }
            } else {
                resolved = rawValue;
                if (allConcrete) {
                    snapshot.emplace_back(paramName, resolved);
                }
            }

            staged.emplace(paramName, std::move(resolved));
        }

        if (allConcrete) {
            auto& slot = ctx->program->getOrCreateCachedState(bindIp);
            slot.cachedTypeArgBindings = std::move(snapshot);
            slot.cachedTypeArgBindingsValid = true;
        }
    }

    void jit_cast_typeparam(value::Value* dest, const value::Value* src,
                             JitContext* ctx,
                             uint32_t paramNameIndex)
    {
        // MYT-228: shared per-frame resolve walk; see TypeExecutor.cpp
        // for the canonical interpreter path.
        const std::string& paramName = ctx->program->getConstantPool().getString(paramNameIndex);
        std::string resolved = paramName;

        if (ctx->vm)
        {
            auto& callStack = ctx->vm->getCallStack();
            for (auto it = callStack.rbegin(); it != callStack.rend(); ++it) {
                if (const auto* p = vm::runtime::utils::resolveTypeParamInFrame(*it, paramName)) {
                    resolved = *p;
                    break;
                }
            }
        }

        if (resolved == "Int" || resolved == "int")
        {
            if (value::isInt(*src))
                { *dest = *src; return; }
            if (value::isFloat(*src))
                { *dest = static_cast<int64_t>(value::asFloat(*src)); return; }
            if (value::isBool(*src))
                { *dest = value::asBool(*src) ? static_cast<int64_t>(1) : static_cast<int64_t>(0); return; }
        }
        else if (resolved == "Float" || resolved == "float")
        {
            if (value::isFloat(*src))
                { *dest = *src; return; }
            if (value::isInt(*src))
                { *dest = static_cast<double>(value::asInt(*src)); return; }
        }
        else if (resolved == "Bool" || resolved == "bool")
        {
            if (value::isBool(*src))
                { *dest = *src; return; }
            if (value::isInt(*src))
                { *dest = (value::asInt(*src) != 0); return; }
        }

        // Phase 1 generics runtime erasure handling. If we reach here for an object type, we just pass it through.
        *dest = *src;
    }

    void jit_new_object(value::Value* dest, JitContext* ctx,
                         uint32_t classIndex, size_t argCount)
    {
        if (ctx->pendingException)
            return;

        try
        {
            const std::string& className = ctx->program->getConstantPool().getString(classIndex);
            // MYT-208: pass a span over ctx->callArgs directly — no per-call
            // heap alloc. The pre-MYT-208 vector copy was O(argCount) and
            // dominated the trivial-ctor hot path on object_alloc-style benches.
            std::span<const value::Value> args(ctx->callArgs, argCount);

            if (ctx->vm)
            {
                *dest = ctx->vm->createObject(className, args);
                return;
            }

            throw errors::RuntimeException("JIT: cannot create object '" + className + "'");
        }
        catch (const OSRDeoptException&)
        {
            throw;
        }
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }

    // MYT-208: JIT helper for NEW_STACK. Calls VirtualMachine::createStackObject
    // which hits the trivial-ctor fast path with acquireRaw + push to current
    // frame's stackObjects + STACK_OBJECT Value emit. Non-trivial ctors fall
    // back to OBJECT (heap) inside createStackObject. The slot type written
    // back is OBJECT for the heap fallback (correct: the produced Value's
    // tag matches whatever createStackObject decided), so downstream IC
    // populates against the right shape.
    void jit_new_stack(value::Value* dest, JitContext* ctx,
                        uint32_t classIndex, size_t argCount)
    {
        if (ctx->pendingException)
            return;

        try
        {
            const std::string& className = ctx->program->getConstantPool().getString(classIndex);
            // Span over ctx->callArgs — zero per-call heap alloc.
            std::span<const value::Value> args(ctx->callArgs, argCount);

            if (ctx->vm)
            {
                *dest = ctx->vm->createStackObject(className, args);
                return;
            }

            throw errors::RuntimeException("JIT: cannot create stack object '" + className + "'");
        }
        catch (const OSRDeoptException&)
        {
            throw;
        }
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }

    void jit_object_to_value(value::Value* val)
    {
        if (!value::isObject(*val))
        {
            return;
        }

        auto instance = value::asObject(*val);
        auto classDef = instance->getClassDefinition();
        auto valueObj = std::make_shared<value::ValueObject>(classDef);

        const auto& fieldIndexMap = classDef->getFieldIndexMap();
        for (const auto& [name, index] : fieldIndexMap)
        {
            valueObj->setFieldByIndex(index, instance->getFieldValue(name));
        }

        for (const auto& [param, type] : instance->getGenericTypeBindings())
        {
            valueObj->setGenericTypeBinding(param, type);
        }

        *val = valueObj;
    }

    static void jitValidateFieldAccess(
        const std::string& fieldName,
        ast::AccessModifier modifier,
        const std::string& targetClassName,
        JitContext* ctx)
    {
        if (modifier == ast::AccessModifier::PUBLIC)
            return;

        // MYT-251: when the access fires inside a JIT-inlined callee body,
        // the access perspective is the callee's owner class, not the
        // outer function's class. The inlined-emit path bumps
        // inlinedCallingClassDepth and writes the callee's owner-class
        // c-string into inlinedCallingClassNames; the helpers below
        // prefer that override. Falls back to callingClassName when
        // depth == 0 (function-level emission, top-level OSR without
        // inlining). Read the c-string into a stack std::string so the
        // existing equality / hierarchy walk works unchanged.
        std::string callerStr;
        const std::string* callerPtr = nullptr;
        bool resolvedFromInlinedStack = false;
        if (ctx->inlinedCallingClassDepth > 0)
        {
            const char* top =
                ctx->inlinedCallingClassNames[ctx->inlinedCallingClassDepth - 1];
            callerStr.assign(top ? top : "");
            callerPtr = &callerStr;
            resolvedFromInlinedStack = true;
        }
        else
        {
            callerPtr = &ctx->callingClassName;
        }
        const std::string& caller = *callerPtr;
        bool isSameClass = (caller == targetClassName);

        if (modifier == ast::AccessModifier::PRIVATE)
        {
            if (!isSameClass)
            {
                std::string callingCtx = caller.empty() ? "global scope" : caller;
                throw errors::AccessViolationException(
                    fieldName, "field", modifier, targetClassName, callingCtx,
                    errors::SourceLocation());
            }
            return;
        }

        if (isSameClass)
            return;

        if (!caller.empty() && ctx->environment)
        {
            auto registry = ctx->environment->getClassRegistry();
            auto currentClass = registry->findClass(caller);
            while (currentClass && currentClass->hasParentClass())
            {
                std::string parentName = currentClass->getParentClassName();
                size_t gPos = parentName.find('<');
                if (gPos != std::string::npos)
                    parentName = parentName.substr(0, gPos);
                if (parentName == targetClassName)
                    return;
                currentClass = registry->findClass(parentName);
            }
        }

        std::string callingCtx = caller.empty() ? "global scope" : caller;
        throw errors::AccessViolationException(
            fieldName, "field", modifier, targetClassName, callingCtx,
            errors::SourceLocation());
    }

    // MYT-208: takes raw ObjectInstance* so STACK_OBJECT receivers (no
    // shared_ptr) can flow through the same field-access path as OBJECT.
    static void validateFieldAccessIfNeeded(
        runtimeTypes::klass::ObjectInstance* instance,
        const std::string& fieldName,
        const runtimeTypes::klass::ClassDefinition* classDef,
        JitContext* ctx)
    {
        auto fieldDef = instance->getField(fieldName);
        if (fieldDef && fieldDef->getAccessModifier() != ast::AccessModifier::PUBLIC)
        {
            auto ownerClass = instance->getClassDefinition()
                ->getFieldOwnerInHierarchy(fieldName, instance->getClassDefinition());
            std::string targetClass = ownerClass ? ownerClass->getName() : classDef->getName();
            jitValidateFieldAccess(fieldName, fieldDef->getAccessModifier(), targetClass, ctx);
        }
    }

    // MYT-167 (F-e follow-up): per-site field IC for ValueObject receivers.
    // ValueObject shares ClassDefinition identity with ObjectInstance, so the
    // same FieldInlineCache (shape → fieldIndex) serves both. IC hit avoids
    // the classDef->getFieldIndex(name) hash lookup that dominated the
    // value-class inlined hot path.
    static bool getFieldFromValueObject(value::Value* dest,
                                        const value::Value* object,
                                        JitContext* ctx,
                                        size_t bytecodeOffset,
                                        uint32_t fieldNameIndex)
    {
        if (!value::isValueObject(*object))
            return false;

        using namespace vm::jit::ic;

        auto valueObj = value::asValueObject(*object);
        auto* classDef = valueObj->getClassDefinition().get();

        // IC fast path — cached shape, known fieldIndex.
        if (ctx->icTable)
        {
            FieldInlineCache& cache = ctx->icTable->getFieldIC(bytecodeOffset);
            if (cache.state == ICState::MONOMORPHIC ||
                cache.state == ICState::POLYMORPHIC)
            {
                const FieldICEntry* entry = cache.lookup(classDef);
                if (entry && entry->fieldIndex != SIZE_MAX &&
                    entry->fieldIndex < valueObj->getFieldCount())
                {
                    *dest = valueObj->getFieldByIndex(entry->fieldIndex);
                    return true;
                }
            }
        }

        // Slow path — resolve field index by name, populate the IC.
        const std::string& fieldName =
            ctx->program->getConstantPool().getString(fieldNameIndex);
        size_t fieldIndex = classDef->getFieldIndex(fieldName);
        if (fieldIndex != SIZE_MAX && fieldIndex < valueObj->getFieldCount())
        {
            *dest = valueObj->getFieldByIndex(fieldIndex);
        }
        else
        {
            *dest = valueObj->getFieldValue(fieldName);
        }

        if (ctx->icTable && fieldIndex != SIZE_MAX)
        {
            FieldInlineCache& cache = ctx->icTable->getFieldIC(bytecodeOffset);
            if (cache.state != ICState::MEGAMORPHIC)
                cache.addEntry(classDef, fieldIndex);
        }
        return true;
    }

    // MYT-208: receiver as raw ObjectInstance* (heap or stack-promoted).
    static bool getFieldICLookupOrFallback(
        value::Value* dest,
        runtimeTypes::klass::ObjectInstance* instance,
        const runtimeTypes::klass::ClassDefinition* classDef,
        JitContext* ctx, size_t bytecodeOffset,
        uint32_t fieldNameIndex)
    {
        using namespace vm::jit::ic;

        if (!ctx->icTable)
            return false;

        FieldInlineCache& cache = ctx->icTable->getFieldIC(bytecodeOffset);

        if (cache.state == ICState::MONOMORPHIC ||
            cache.state == ICState::POLYMORPHIC)
        {
            const FieldICEntry* entry = cache.lookup(classDef);
            if (entry && entry->fieldIndex != SIZE_MAX)
            {
                if (!instance->hasFieldVector())
                    instance->ensureFieldVector();
                *dest = instance->getFieldByIndex(entry->fieldIndex);
                return true;
            }
        }

        const std::string& fieldName =
            ctx->program->getConstantPool().getString(fieldNameIndex);

        validateFieldAccessIfNeeded(instance, fieldName, classDef, ctx);

        *dest = instance->getFieldValue(fieldName);

        if (cache.state != ICState::MEGAMORPHIC)
        {
            size_t fieldIndex = classDef->getFieldIndex(fieldName);
            if (fieldIndex != SIZE_MAX)
            {
                if (!instance->hasFieldVector())
                    instance->ensureFieldVector();
                cache.addEntry(classDef, fieldIndex);
            }
        }
        return true;
    }

    void jit_get_field_ic(value::Value* dest, const value::Value* object,
                          JitContext* ctx, size_t bytecodeOffset,
                          uint32_t fieldNameIndex, uint8_t flags)
    {
        if (!(flags & bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER))
        {
            if (vm::runtime::utils::isNullValue(*object))
            {
                throw errors::NullPointerException("Cannot access field on null");
            }
        }

        if (getFieldFromValueObject(dest, object, ctx, bytecodeOffset, fieldNameIndex))
            return;

        // MYT-208: raw unwrap covers OBJECT and STACK_OBJECT receivers.
        auto* instance = value::asObjectInstanceRaw(*object);
        auto* classDef = instance->getClassDefinition().get();

        if (getFieldICLookupOrFallback(dest, instance, classDef, ctx,
                                        bytecodeOffset, fieldNameIndex))
            return;

        const std::string& fieldName =
            ctx->program->getConstantPool().getString(fieldNameIndex);

        validateFieldAccessIfNeeded(instance, fieldName, classDef, ctx);

        *dest = instance->getFieldValue(fieldName);
    }

    static bool setFieldOnValueObject(value::Value* destValue,
                                      const value::Value* object,
                                      const value::Value* newValue,
                                      JitContext* ctx,
                                      uint32_t fieldNameIndex)
    {
        if (!value::isValueObject(*object))
            return false;

        auto valueObj = value::asValueObject(*object);
        auto copy = valueObj->deepCopy();
        const std::string& fieldName =
            ctx->program->getConstantPool().getString(fieldNameIndex);
        copy->setField(fieldName, *newValue);
        *const_cast<value::Value*>(object) = copy;
        *destValue = *newValue;
        return true;
    }

    // MYT-208: receiver as raw ObjectInstance* (heap or stack-promoted).
    static bool setFieldICLookupOrFallback(
        value::Value* destValue,
        runtimeTypes::klass::ObjectInstance* instance,
        const runtimeTypes::klass::ClassDefinition* classDef,
        const value::Value* newValue,
        JitContext* ctx, size_t bytecodeOffset,
        uint32_t fieldNameIndex)
    {
        using namespace vm::jit::ic;

        if (!ctx->icTable)
            return false;

        FieldInlineCache& cache = ctx->icTable->getFieldIC(bytecodeOffset);

        if (cache.state == ICState::MONOMORPHIC ||
            cache.state == ICState::POLYMORPHIC)
        {
            const FieldICEntry* entry = cache.lookup(classDef);
            if (entry && entry->fieldIndex != SIZE_MAX)
            {
                if (!instance->hasFieldVector())
                    instance->ensureFieldVector();
                instance->setFieldByIndex(entry->fieldIndex, *newValue);
                *destValue = *newValue;
                return true;
            }
        }

        const std::string& fieldName =
            ctx->program->getConstantPool().getString(fieldNameIndex);

        validateFieldAccessIfNeeded(instance, fieldName, classDef, ctx);

        instance->setField(fieldName, *newValue);
        *destValue = *newValue;

        if (cache.state != ICState::MEGAMORPHIC)
        {
            size_t fieldIndex = classDef->getFieldIndex(fieldName);
            if (fieldIndex != SIZE_MAX)
            {
                if (!instance->hasFieldVector())
                    instance->ensureFieldVector();
                cache.addEntry(classDef, fieldIndex);
            }
        }
        return true;
    }

    void jit_set_field_ic(value::Value* destValue, const value::Value* object,
                          const value::Value* newValue,
                          JitContext* ctx, size_t bytecodeOffset,
                          uint32_t fieldNameIndex, uint8_t flags)
    {
        if (!(flags & bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER))
        {
            if (vm::runtime::utils::isNullValue(*object))
            {
                throw errors::NullPointerException("Cannot set field on null");
            }
        }

        if (setFieldOnValueObject(destValue, object, newValue, ctx, fieldNameIndex))
            return;

        // MYT-208: raw unwrap covers OBJECT and STACK_OBJECT receivers.
        auto* instance = value::asObjectInstanceRaw(*object);
        auto* classDef = instance->getClassDefinition().get();

        if (setFieldICLookupOrFallback(destValue, instance, classDef, newValue,
                                        ctx, bytecodeOffset, fieldNameIndex))
            return;

        const std::string& fieldName =
            ctx->program->getConstantPool().getString(fieldNameIndex);

        validateFieldAccessIfNeeded(instance, fieldName, classDef, ctx);

        instance->setField(fieldName, *newValue);
        *destValue = *newValue;
    }

    // MYT-274 Phase 2 v2: structural-equality JIT helpers. Mirror the
    // interpreter executors in ObjectExecutor::handleStructHashInt /
    // handleStructEqInt — same Horner accumulation, same isClassOf gate.
    // Called from JIT-emitted code so methods bodies that reduce to a
    // single STRUCT_HASH_INT / STRUCT_EQ_INT can be JIT-compiled and
    // (with the InlineAnalysis allow-list update below) inlined into hot
    // callers like HashMap.containsKey.

    int64_t jit_struct_hash_int(const value::Value* receiver,
                                uint64_t fieldCount,
                                const uint64_t* slots)
    {
        if (!receiver || (fieldCount > 0 && !slots)) return 1;

        int64_t h = 1;

        if (auto* raw = value::asObjectInstanceRaw(*receiver))
        {
            if (!raw->hasFieldVector()) return h;
            for (uint64_t i = 0; i < fieldCount; ++i)
            {
                const value::Value& fv = raw->getFieldByIndex(static_cast<size_t>(slots[i]));
                const int64_t iv = value::isInt(fv) ? value::asInt(fv) : 0;
                h = h * 31 + iv;
            }
            return h;
        }

        if (value::isValueObject(*receiver))
        {
            auto vobj = value::asValueObject(*receiver);
            for (uint64_t i = 0; i < fieldCount; ++i)
            {
                const value::Value& fv = vobj->getFieldByIndex(static_cast<size_t>(slots[i]));
                const int64_t iv = value::isInt(fv) ? value::asInt(fv) : 0;
                h = h * 31 + iv;
            }
        }
        return h;
    }

    // Returns 1 for equal, 0 for not equal. asmjit's Gp is 64-bit so we
    // pass the bool result back through int64_t to match the helper's
    // FuncSignature. The caller writes it directly into the BOOL stack
    // slot; only the low bit is read by downstream consumers.
    int64_t jit_struct_eq_int(const value::Value* thisVal,
                              const value::Value* otherVal,
                              const char* className,
                              uint64_t fieldCount,
                              const uint64_t* slots)
    {
        if (!thisVal || !otherVal || !className) return 0;
        if (value::isNullType(*otherVal)) return 0;

        auto* thisRaw = value::asObjectInstanceRaw(*thisVal);
        auto* otherRaw = value::asObjectInstanceRaw(*otherVal);
        if (!thisRaw || !otherRaw) return 0;
        if (!thisRaw->hasFieldVector() || !otherRaw->hasFieldVector()) return 0;

        const std::string& otherClassName = otherRaw->getClassDefinition()->getName();
        const std::string ownerClass(className);
        if (otherClassName != ownerClass)
        {
            // Exact class match short-circuits the subclass walk for the
            // common HashMap<UserClass, V> case where every key is the
            // same class. Walk the parent chain only when the names
            // differ — covers the rare polymorphic-key scenario.
            // getParentClass() already returns a locked shared_ptr; we
            // hold it across each iteration to keep the def alive.
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> defHolder =
                otherRaw->getClassDefinition();
            bool match = false;
            while (defHolder)
            {
                if (defHolder->getName() == ownerClass) { match = true; break; }
                defHolder = defHolder->getParentClass();
            }
            if (!match) return 0;
        }

        for (uint64_t i = 0; i < fieldCount; ++i)
        {
            const value::Value& a = thisRaw->getFieldByIndex(static_cast<size_t>(slots[i]));
            const value::Value& b = otherRaw->getFieldByIndex(static_cast<size_t>(slots[i]));
            const int64_t ai = value::isInt(a) ? value::asInt(a) : 0;
            const int64_t bi = value::isInt(b) ? value::asInt(b) : 0;
            if (ai != bi) return 0;
        }
        return 1;
    }
}
