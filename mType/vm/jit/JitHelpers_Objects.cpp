#include "JitHelpers.hpp"
#include "JitCodeCache.hpp"
#include "ic/InlineCacheTable.hpp"
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
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*receiver))
        {
            const auto& instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*receiver);
            return instance ? instance->getClassDefinition().get() : nullptr;
        }
        if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(*receiver))
        {
            const auto& valueObj = std::get<std::shared_ptr<value::ValueObject>>(*receiver);
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

            if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue))
            {
                auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);

                if (ctx->vm)
                {
                    ctx->returnValue = ctx->vm->callMethodFromJit(instance, methodName, args);
                    ctx->hasReturnValue = true;
                    return;
                }
            }

            if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(objectValue))
            {
                auto valueObj = std::get<std::shared_ptr<value::ValueObject>>(objectValue);
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

    // MYT-132: direct JIT→JIT dispatch for method calls on IC hit.
    // If the callee has a JIT entry in jitCodeCache, invoke it directly —
    // skipping the interpreter loop in callMethodFromJitDirect. The emitter
    // (emitCallMethodOp) has already marshalled receiver into ctx->callArgs[0]
    // and args into ctx->callArgs[1..argCount], which matches the layout the
    // callee's emitArgumentUnboxing expects (args[0]=this, args[i]=arg_i).
    //
    // entry->jitEntry is the cached JIT entry pointer (populated on IC
    // create and refreshed lazily here). Avoids a per-call jitCodeCache
    // hash-string lookup on the 2M-calls-per-loop hot path.
    static bool tryDirectJitMethodDispatch(
        JitContext* ctx,
        const ic::MethodICEntry* entry,
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance,
        size_t methodArgCount)
    {
        if (!ctx->vm)
            return false;

        // MYT-182: refuse direct JIT→JIT dispatch across programs. The
        // nested JIT code reads constant-pool strings / type indices via
        // nestedCtx.program — a mismatched program would hand it the wrong
        // data or OOB. The caller falls through to callMethodFromJitDirect,
        // which properly saves / switches / restores executionCtx->program.
        if (entry->program && entry->program != ctx->program)
            return false;

        // Fast path: cached JIT entry from IC populate / previous refresh.
        auto jitFn = reinterpret_cast<JitFunction>(entry->jitEntry);
        if (!jitFn)
        {
            // Slow path: callee may have just become hot and JIT-compiled
            // after this IC was first populated. Retry the lookup once and
            // cache the result for subsequent calls.
            if (!ctx->jitCodeCache)
                return false;
            jitFn = ctx->jitCodeCache->lookup(entry->qualifiedName);
            if (!jitFn)
                return false;
            entry->jitEntry = reinterpret_cast<ic::JitEntryPtr>(jitFn);
        }

        if (ctx->vm->getJitNativeDepth() >= vm::runtime::VirtualMachine::MAX_JIT_NATIVE_DEPTH)
            return false;

        const std::string& qualifiedName = entry->qualifiedName;

        vm::runtime::CallFrame frame;
        frame.returnAddress = 0;
        frame.frameBase = 0;
        frame.localBase = 0;
        frame.functionName = qualifiedName;
        frame.thisInstance = instance;
        size_t sepPos = qualifiedName.find("::");
        if (sepPos != std::string::npos)
            frame.definingClassName = qualifiedName.substr(0, sepPos);

        ctx->vm->pushCallFrame(frame);
        ctx->vm->incrementJitNativeDepth();

        JitContext nestedCtx{};
        nestedCtx.args = ctx->callArgs;
        nestedCtx.argCount = methodArgCount + 1;  // +1 for `this` receiver
        nestedCtx.hasReturnValue = false;
        nestedCtx.program = ctx->program;
        nestedCtx.stackManager = ctx->stackManager;
        nestedCtx.environment = ctx->environment;
        nestedCtx.vm = ctx->vm;
        nestedCtx.jitCodeCache = ctx->jitCodeCache;
        nestedCtx.icTable = ctx->icTable;
        nestedCtx.callingClassName = frame.definingClassName;

        jitFn(&nestedCtx);

        ctx->vm->decrementJitNativeDepth();
        ctx->vm->popCallStack();

        if (nestedCtx.pendingException)
        {
            ctx->pendingException = nestedCtx.pendingException;
            return true;
        }

        ctx->returnValue = nestedCtx.hasReturnValue
            ? nestedCtx.returnValue : value::Value{std::monostate{}};
        ctx->hasReturnValue = true;
        return true;
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

            // MYT-167 (F-e): unified receiver-kind handling. ValueObject
            // receivers populate the IC (with receiverIsValueObject=true) and,
            // on hit, route dispatch through jit_call_method (which performs
            // the temp-ObjectInstance materialisation). The speculative
            // inliner later consumes the IC entry and emits a direct inlined
            // body for eligible read-only callees.
            const runtimeTypes::klass::ClassDefinition* classDef = nullptr;
            bool receiverIsValueObject = false;
            std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance;

            if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue))
            {
                instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(receiverValue);
                if (!instance)
                {
                    jit_call_method(ctx, methodNameIndex, argCount);
                    return;
                }
                classDef = instance->getClassDefinition().get();
            }
            else if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(receiverValue))
            {
                const auto& valueObj = std::get<std::shared_ptr<value::ValueObject>>(receiverValue);
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
                        // Try direct JIT→JIT dispatch first. Skips the interpreter
                        // loop in callMethodFromJitDirect when the callee is
                        // JIT-compiled. Falls through on lookup miss / depth limit.
                        if (tryDirectJitMethodDispatch(ctx, entry, instance, argCount))
                            return;

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
                        // MYT-161: pre-populate cached JIT entry pointer if
                        // the callee is already JIT-compiled. Only meaningful
                        // for ObjectInstance direct dispatch — ValueObject
                        // hits always go through jit_call_method.
                        if (!receiverIsValueObject && ctx->jitCodeCache)
                        {
                            auto jitFn = ctx->jitCodeCache->lookup(resolution.qualifiedName);
                            if (jitFn)
                                entry.jitEntry = reinterpret_cast<ic::JitEntryPtr>(jitFn);
                        }
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
            return std::holds_alternative<int64_t>(*val) ? 1 : 0;
        if (typeName == "Float" || typeName == "float")
            return std::holds_alternative<double>(*val) ? 1 : 0;
        if (typeName == "Bool" || typeName == "bool")
            return std::holds_alternative<bool>(*val) ? 1 : 0;
        if (typeName == "String" || typeName == "string")
            return (std::holds_alternative<std::string>(*val) ||
                    std::holds_alternative<value::InternedString>(*val)) ? 1 : 0;

        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*val))
        {
            auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*val);
            auto classDef = instance->getClassDefinition();
            while (classDef)
            {
                if (classDef->getName() == typeName) return 1;
                classDef = classDef->getParentClass();
            }
        }

        if (std::holds_alternative<std::shared_ptr<value::ValueObject>>(*val))
        {
            auto valueObj = std::get<std::shared_ptr<value::ValueObject>>(*val);
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
            if (std::holds_alternative<int64_t>(*src))
                { *dest = *src; return; }
            if (std::holds_alternative<double>(*src))
                { *dest = static_cast<int64_t>(std::get<double>(*src)); return; }
            if (std::holds_alternative<bool>(*src))
                { *dest = std::get<bool>(*src) ? static_cast<int64_t>(1) : static_cast<int64_t>(0); return; }
        }
        else if (targetType == "Float" || targetType == "float")
        {
            if (std::holds_alternative<double>(*src))
                { *dest = *src; return; }
            if (std::holds_alternative<int64_t>(*src))
                { *dest = static_cast<double>(std::get<int64_t>(*src)); return; }
        }
        else if (targetType == "Bool" || targetType == "bool")
        {
            if (std::holds_alternative<bool>(*src))
                { *dest = *src; return; }
            if (std::holds_alternative<int64_t>(*src))
                { *dest = (std::get<int64_t>(*src) != 0); return; }
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
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*val))
        {
            return;
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*val);
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
        if (!std::holds_alternative<std::shared_ptr<value::ValueObject>>(*object))
            return false;

        using namespace vm::jit::ic;

        auto valueObj = std::get<std::shared_ptr<value::ValueObject>>(*object);
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

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*object);
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
        if (!std::holds_alternative<std::shared_ptr<value::ValueObject>>(*object))
            return false;

        auto valueObj = std::get<std::shared_ptr<value::ValueObject>>(*object);
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

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(*object);
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
