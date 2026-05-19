#include "JitHelpers.hpp"
#include "ic/InlineCacheTable.hpp"
#include "../../value/ValueShim.hpp"
#include "../../errors/AccessViolationException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../runtime/utils/NullCheckUtils.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
#include "../../environment/Environment.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include "../../value/ValueObject.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>

namespace vm::jit
{
    // Shape-guard helper. Extracts the raw ClassDefinition pointer from a
    // receiver, handling both ObjectInstance (including STACK_OBJECT — same
    // ClassDef offset) and ValueObject variants. Returns nullptr for other
    // variants so the guard mismatches and the inlined site falls back to
    // jit_call_method_ic. Keeping the shared_ptr layout behind this helper
    // avoids baking MSVC control-block internals into emitted code.
    const void* jit_extract_classdef(const value::Value* receiver)
    {
        if (!receiver) return nullptr;
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

    // Inline-IC SET fast path; thin wrapper around ObjectInstance::setFieldByIndex
    // preserving the full write-barrier sequence (oldPtr decref, cycle-root mark,
    // fieldValues map sync) while skipping IC lookup and name resolution.
    // The emitter gates on MONOMORPHIC + non-value-class. Anything unexpected
    // at runtime signals via a false return so the caller jumps to the slow
    // path (jit_set_field_ic).
    bool jit_field_set_at(value::Value* destSlot,
                          const value::Value* receiverSlot,
                          size_t fieldIndex,
                          const value::Value* newValue) noexcept
    {
        if (!destSlot || !receiverSlot || !newValue) return false;
        if (!value::isAnyObject(*receiverSlot)) return false;
        auto* raw = value::asObjectInstanceRaw(*receiverSlot);
        if (!raw) return false;
        // Pin `raw` before any operation that could overwrite *destSlot. When
        // destSlot == receiverSlot (the common case — emitter passes the same
        // pointer for both), writing through destSlot for an OBJECT receiver
        // releases the shared_ptr that holds the only strong ref to this
        // instance and frees the object. Sequencing: touch `raw` first, slot
        // overwrite strictly last.
        if (!raw->hasFieldVector())
            raw->ensureFieldVector();
        const auto& classDef = raw->getClassDefinition();
        if (!classDef || fieldIndex >= classDef->getTotalFieldCount())
            return false;
        raw->setFieldByIndex(fieldIndex, *newValue);
        // Last access to the instance completed above. Mirrors jit_set_field_ic's
        // `*destValue = *newValue` semantics. DO NOT reference `raw` past here —
        // the backing object may be freed.
        *destSlot = *newValue;
        return true;
    }

    static void jitValidateFieldAccess(
        const std::string& fieldName,
        ast::AccessModifier modifier,
        const std::string& targetClassName,
        JitContext* ctx)
    {
        if (modifier == ast::AccessModifier::PUBLIC)
            return;

        // When the access fires inside a JIT-inlined callee body, the access
        // perspective is the callee's owner class, not the outer function's.
        // emitInlineCalleeBody bumps inlinedCallingClassDepth and writes the
        // callee's owner-class c-string into inlinedCallingClassNames; prefer
        // that override and fall back to callingClassName at depth 0.
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

    // Takes raw ObjectInstance* so STACK_OBJECT receivers (no shared_ptr) flow
    // through the same field-access path as OBJECT.
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

    // Per-site field IC for ValueObject receivers. ValueObject shares
    // ClassDefinition identity with ObjectInstance, so the same FieldInlineCache
    // (shape → fieldIndex) serves both. IC hit avoids the
    // classDef->getFieldIndex(name) hash lookup that dominated the value-class
    // inlined hot path.
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

    // Receiver as raw ObjectInstance* (heap or stack-promoted).
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

    // Receiver as raw ObjectInstance* (heap or stack-promoted).
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
}
