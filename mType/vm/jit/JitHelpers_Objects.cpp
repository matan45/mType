#include "JitHelpers.hpp"
#include "JitCodeCache.hpp"
#include "ic/InlineCacheTable.hpp"
#include "../../value/ValueShim.hpp"
#include "../../errors/AccessViolationException.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../runtime/utils/NullCheckUtils.hpp"
#include "../runtime/utils/MethodResolver.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
#include "../../environment/Environment.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/klass/SignatureUtils.hpp"
#include "../../value/ObjectInstancePool.hpp"
#include "../../value/ValueObject.hpp"
#include <vector>
#include <memory>

namespace vm::jit
{
    // MYT-163: speculative inlining shape-guard helper. Extracts the raw
    // ClassDefinition pointer from a receiver, handling both ObjectInstance
    // and ValueObject variants (MYT-167 F-e). Returns nullptr for any other
    // variant so the guard compare mismatches and the inlined site falls back
    // to jit_call_method_ic. Keeping the shared_ptr layout behind this helper
    // avoids baking MSVC control-block internals into emitted code.
    const void* jit_extract_classdef(const value::Value* receiver)
    {
        if (!receiver) return nullptr;
        if (value::isObject(*receiver))
        {
            const auto& instance = value::asObject(*receiver);
            return instance ? instance->getClassDefinition().get() : nullptr;
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
        if (value::isObject(*receiver))
        {
            const auto& instance = value::asObject(*receiver);
            if (!instance) return nullptr;
            if (!instance->hasFieldVector()) return nullptr;
            return &instance->getFieldByIndex(0);
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
        if (!value::isObject(*receiverSlot)) return false;
        const auto& instance = value::asObject(*receiverSlot);
        if (!instance) return false;
        // Pin the raw ObjectInstance pointer BEFORE any operation that could
        // overwrite *destSlot. When destSlot == receiverSlot (the common case —
        // emitter passes the same pointer for both), writing through destSlot
        // releases the shared_ptr that currently holds the only strong ref to
        // this instance, dropping its refcount to zero and freeing the object.
        // Any use of `instance` after the overwrite would be a use-after-free.
        // Sequencing below: all work that touches `instance` runs first; the
        // slot overwrite is strictly the last action. Pinning here makes the
        // lifetime contract explicit and robust against future insertions
        // between the setFieldByIndex call and *destSlot = *newValue.
        auto* raw = instance.get();
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
            const std::string& methodName = ctx->program->getConstantPool().getString(methodNameIndex);

            value::Value& objectValue = ctx->callArgs[0];

            std::vector<value::Value> args;
            for (size_t i = 1; i <= argCount; i++)
            {
                args.push_back(ctx->callArgs[i]);
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

            throw errors::RuntimeException("JIT: cannot call method '" + methodName + "'");
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
            std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance;

            if (value::isObject(receiverValue))
            {
                instance = value::asObject(receiverValue);
                if (!instance)
                {
                    jit_call_method(ctx, methodNameIndex, argCount);
                    return;
                }
                classDef = instance->getClassDefinition().get();
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
                // MYT-181: strip class prefix + signature suffix. The compiler
                // emits mangled names; findInstanceMethodCached expects the
                // simple name.
                const std::string simpleMethodName =
                    runtimeTypes::klass::SignatureUtils::extractSimpleName(rawMethodName);
                auto lookupResult = classDef->findInstanceMethodCached(simpleMethodName, argCount);
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

        if (value::isObject(*val))
        {
            auto instance = value::asObject(*val);
            auto classDef = instance->getClassDefinition();
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

    void jit_new_object(value::Value* dest, JitContext* ctx,
                         uint32_t classIndex, size_t argCount)
    {
        if (ctx->pendingException)
            return;

        try
        {
            const std::string& className = ctx->program->getConstantPool().getString(classIndex);
            std::vector<value::Value> args(ctx->callArgs, ctx->callArgs + argCount);

            if (ctx->vm)
            {
                *dest = ctx->vm->createObject(className, args);
                return;
            }

            throw errors::RuntimeException("JIT: cannot create object '" + className + "'");
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

        const std::string& caller = ctx->callingClassName;
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

    static void validateFieldAccessIfNeeded(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance,
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

    static bool getFieldICLookupOrFallback(
        value::Value* dest,
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance,
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

        auto instance = value::asObject(*object);
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

    static bool setFieldICLookupOrFallback(
        value::Value* destValue,
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance,
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

        auto instance = value::asObject(*object);
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
}
