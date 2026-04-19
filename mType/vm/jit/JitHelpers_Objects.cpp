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
                auto valueObj = value::asValueObject(objectValue);
                auto classDef = valueObj->getClassDefinition();

                auto tempInstance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDef);
                const auto& fieldIndexMap = classDef->getFieldIndexMap();
                for (const auto& [name, index] : fieldIndexMap)
                {
                    if (index < valueObj->getFieldCount())
                    {
                        tempInstance->setField(name, valueObj->getFieldByIndex(index));
                    }
                }

                if (ctx->vm)
                {
                    ctx->returnValue = ctx->vm->callMethodFromJit(tempInstance, methodName, args);
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

            // MYT-167 (F-e): unified receiver-kind handling. ValueObject
            // receivers populate the IC (with receiverIsValueObject=true) and,
            // on hit, route dispatch through jit_call_method (which performs
            // the temp-ObjectInstance materialisation). The speculative
            // inliner later consumes the IC entry and emits a direct inlined
            // body for eligible read-only callees.
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
                    if (!receiverIsValueObject)
                    {
                        // MYT-184: route ObjectInstance method IC hits through the
                        // mini-interpret loop. Direct JIT->JIT dispatch was removed
                        // after it caused /GS stack-cookie corruption (see the
                        // comment block above this function).
                        auto* funcMeta = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(entry->funcMetadata);
                        std::vector<value::Value> args;
                        args.reserve(argCount);
                        for (size_t i = 1; i <= argCount; ++i)
                        {
                            args.push_back(ctx->callArgs[i]);
                        }
                        // MYT-182: pass the callee's program so the mini-interpret
                        // loop in callMethodFromJitDirect runs against the right
                        // bytecode for library callees.
                        ctx->returnValue = ctx->vm->callMethodFromJitDirect(
                            instance, entry->qualifiedName, funcMeta, args,
                            entry->program);
                        ctx->hasReturnValue = true;
                        return;
                    }

                    // ValueObject IC hit — generic dispatch. callMethodFromJitDirect
                    // requires shared_ptr<ObjectInstance>; the temp-materialisation
                    // branch in jit_call_method handles ValueObject correctly.
                    // The inliner consumes this IC entry on the caller's next
                    // recompile, so this slow path runs only between populate
                    // and recompile.
                    jit_call_method(ctx, methodNameIndex, argCount);
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
