#include "JitHelpers.hpp"
#include "JitHelpers_Objects.hpp"
#include "JitCodeCache.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "ic/InlineCacheTable.hpp"
#include "../../value/ValueShim.hpp"
#include "../../value/HashUtils.hpp"
#include "../../value/PrimitiveTypeTag.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../runtime/utils/MethodResolver.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include "../../environment/registry/SignatureUtils.hpp"
#include "../../value/ValueObject.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

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
                out = ::value::hashutils::intHash(value::asInt(*field));
                return true;
            case value::PrimitiveTypeTag::FLOAT:
                if (!value::isFloat(*field)) return false;
                out = ::value::hashutils::floatHash(value::asFloat(*field));
                return true;
            case value::PrimitiveTypeTag::BOOL:
                if (!value::isBool(*field)) return false;
                out = ::value::hashutils::boolHash(value::asBool(*field));
                return true;
            case value::PrimitiveTypeTag::STRING:
                if (value::isAnyString(*field))
                {
                    out = ::value::hashutils::stringHash(value::asStringView(*field));
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

    // Direct JIT->JIT method dispatch recreates the runtime boundary that
    // callMethodFromJitDirect used to provide. A raw function pointer isn't
    // enough: library callees must run with their owning BytecodeProgram
    // (constant pool, metadata, nested IC lookups) and the VM call stack
    // needs a method frame while the nested JIT body is active so exception
    // handling, access checks, and stack traces see the callee.
    void jit_call_method_direct(JitContext* ctx,
                                 const void* cachedJit,
                                 const bytecode::BytecodeProgram* calleeProgram,
                                 const void* funcMetadata,
                                 size_t argCountPlusReceiver)
    {
        if (ctx->pendingException) return;
        if (!cachedJit || !ctx->vm) return;
        const auto* funcMeta =
            static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(funcMetadata);
        if (!calleeProgram || !funcMeta) return;

        // Mirror tryJitDispatchResolved's native-recursion guard.
        if (ctx->vm->getJitNativeDepth() >= vm::runtime::VirtualMachine::MAX_JIT_NATIVE_DEPTH)
        {
            try {
                throw errors::RuntimeException(
                    "JIT: native recursion depth exceeded in direct dispatch");
            } catch (...) {
                ctx->pendingException = std::current_exception();
            }
            return;
        }

        auto fn = reinterpret_cast<void(*)(JitContext*)>(const_cast<void*>(cachedJit));
        const std::string& qualifiedName = funcMeta->mangledName.empty()
            ? funcMeta->name : funcMeta->mangledName;

        size_t calleeProgramIndex = 0;
        const auto& loadedPrograms = ctx->vm->getLoadedPrograms();
        if (!ctx->vm->getCallStack().empty())
            calleeProgramIndex = ctx->vm->getCallStack().back().programIndex;
        for (size_t i = 0; i < loadedPrograms.size(); ++i)
        {
            if (loadedPrograms[i] == calleeProgram)
            {
                calleeProgramIndex = i;
                break;
            }
        }

        std::string definingClassName;
        size_t colonPos = qualifiedName.find("::");
        if (colonPos != std::string::npos)
            definingClassName = qualifiedName.substr(0, colonPos);

        const size_t savedStackSize = ctx->stackManager ? ctx->stackManager->size() : 0;
        vm::runtime::CallFrame frame;
        frame.returnAddress = ctx->vm->getInstructionPointer();
        frame.frameBase = savedStackSize;
        frame.localBase = savedStackSize;
        frame.functionName = calleeProgram->internFrameName(qualifiedName);
        frame.definingClassName = definingClassName;
        frame.programIndex = calleeProgramIndex;
        if (argCountPlusReceiver > 0)
        {
            const value::Value& receiver = ctx->callArgs[0];
            if (value::isObject(receiver))
                frame.thisInstance = value::asObject(receiver);
            else if (value::isStackObject(receiver))
                frame.thisInstanceRaw = value::asObjectInstanceRaw(receiver);
        }
        ctx->vm->pushCallFrame(std::move(frame));
        bool framePushed = true;

        JitContext nestedCtx{};
        nestedCtx.args = ctx->callArgs;
        nestedCtx.argCount = argCountPlusReceiver;
        nestedCtx.hasReturnValue = false;
        nestedCtx.program = calleeProgram;
        nestedCtx.stackManager = ctx->stackManager;
        nestedCtx.environment = ctx->environment;
        nestedCtx.vm = ctx->vm;
        nestedCtx.jitCodeCache = ctx->jitCodeCache;
        nestedCtx.icTable = ctx->icTable;
        nestedCtx.callingClassName = definingClassName.empty()
            ? ctx->callingClassName : definingClassName;

        ctx->vm->incrementJitNativeDepth();
        try {
            fn(&nestedCtx);
        } catch (...) {
            ctx->pendingException = std::current_exception();
        }
        ctx->vm->decrementJitNativeDepth();
        if (framePushed)
        {
            ctx->vm->popCallStack();
            framePushed = false;
        }

        if (nestedCtx.pendingException && !ctx->pendingException) {
            ctx->pendingException = nestedCtx.pendingException;
            return;
        }
        if (ctx->pendingException) return;

        ctx->returnValue = std::move(nestedCtx.returnValue);
        ctx->hasReturnValue = nestedCtx.hasReturnValue;
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

            const std::string& rawMethodName =
                ctx->program->getConstantPool().getString(methodNameIndex);
            if (trySpecializedCollectionCall(ctx, rawMethodName, argCount))
            {
                return;
            }

            // Unified receiver-kind handling: ObjectInstance and ValueObject hits
            // both route through callMethodFromJitDirect(Value). The Value-receiver
            // overload batch-materialises a temp ObjectInstance for value-class
            // receivers and delegates to the shared_ptr path so the callee runs
            // through one mini-interpret loop regardless of receiver kind.
            const runtimeTypes::klass::ClassDefinition* classDef = nullptr;
            bool receiverIsValueObject = false;

            // OBJECT and STACK_OBJECT share the same raw-unwrap path; the IC
            // shape key is ClassDefinition*, identical for both.
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

            // Fast path: monomorphic / polymorphic / wide-tier shape match.
            // MEGAMORPHIC also consults the cache; lookup() walks the wide tier
            // on top of the inline POLY array, so overflow shapes 5..16 still
            // get a cached dispatch from this JIT helper instead of falling
            // through to the runtime method resolver.
            if (cache.state == ic::ICState::MONOMORPHIC ||
                cache.state == ic::ICState::POLYMORPHIC ||
                cache.state == ic::ICState::MEGAMORPHIC)
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
                    // Pre-resolved funcMetadata skips the findInstanceMethodInHierarchy
                    // + program->getFunction probes the slow jit_call_method path
                    // would redo on every iteration. The cachedJit slot is
                    // populated by the IC-fill path below; if the callee tiers up
                    // AFTER fill, we refresh lazily on first observation so the
                    // win is captured on the very next iteration.
                    auto* funcMeta = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(entry->funcMetadata);

                    const void* directTarget = entry->cachedJit;
                    if (!directTarget && !entry->receiverIsValueObject)
                    {
                        if (auto* codeCache = ctx->vm->getJitCodeCache())
                        {
                            directTarget = reinterpret_cast<const void*>(
                                codeCache->lookup(entry->qualifiedName));
                            if (directTarget)
                            {
                                // Write back to the matching mutable entry so
                                // subsequent hits skip the probe.
                                for (uint8_t i = 0; i < cache.entryCount; ++i)
                                {
                                    if (cache.entries[i].shape == classDef)
                                    {
                                        cache.entries[i].cachedJit = directTarget;
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    if (directTarget && !entry->receiverIsValueObject)
                    {
                        // callArgs[0] is already the receiver and [1..argCount]
                        // are the args — same layout jit_call_method_direct
                        // expects (argCount+1 params).
                        jit_call_method_direct(ctx, directTarget,
                                                entry->program,
                                                entry->funcMetadata,
                                                argCount + 1);
                        return;
                    }

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

            // Miss — defer to the generic helper.
            jit_call_method(ctx, methodNameIndex, argCount);
            if (ctx->pendingException)
                return;

            // Populate the cache on every state including MEGAMORPHIC.
            // addEntry routes shapes past the inline POLY tier into the wide
            // tier; the next call of that shape will hit the cached dispatch
            // above instead of repeating the runtime resolution.
            {
                const std::string& rawMethodName =
                    ctx->program->getConstantPool().getString(methodNameIndex);
                const std::string simpleMethodName =
                    runtimeTypes::klass::SignatureUtils::extractSimpleName(rawMethodName);
                auto lookupResult =
                    classDef->findInstanceMethodForCallNameCached(rawMethodName, argCount);
                if (lookupResult.method)
                {
                    // Resolve across main + loaded library programs so the IC
                    // entry knows which program owns funcMetadata.
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
                        // Flag reflects the observed receiver kind.
                        // InlineAnalysis::scanCalleeOpcodes consumes this to
                        // apply the read-only restriction for value-class sites.
                        entry.receiverIsValueObject = receiverIsValueObject;
                        entry.protocolFastKind = classifyPrimitiveProtocolFastKind(
                            classDef, simpleMethodName, argCount,
                            lookupResult.definingClassName);
                        // Cache the callee's JitFunction pointer if it's already
                        // JIT-compiled.
                        if (ctx->vm) {
                            if (auto* codeCache = ctx->vm->getJitCodeCache()) {
                                entry.cachedJit = reinterpret_cast<const void*>(
                                    codeCache->lookup(entry.qualifiedName));
                            }
                        }
                        // Re-fetch the cache reference immediately before the
                        // write. Nested CALL_METHODs in jit_call_method above
                        // may have inserted new entries and rehashed methodCaches;
                        // the captured reference is not pointer-stable across that.
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
}
