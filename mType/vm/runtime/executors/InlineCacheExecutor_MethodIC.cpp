#include "InlineCacheExecutor.hpp"
#include "ObjectExecutor.hpp"
#include "../VirtualMachine.hpp"
#include "../utils/MethodResolver.hpp"
#include "../../../value/HashUtils.hpp"
#include "../../../value/SmallArgsBuffer.hpp"
#include "../../../value/PrimitiveTypeTag.hpp"
#include "../../../environment/registry/SignatureUtils.hpp"
#include "../../jit/JitCodeCache.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace vm::runtime
{
    namespace
    {
        value::PrimitiveTypeTag getPrimitiveProtocolTag(const value::Value& v) noexcept
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

        const value::Value* getPrimitiveProtocolField0(const value::Value& v) noexcept
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

        bool computePrimitiveProtocolHash(value::Value& out, const value::Value& receiver)
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
                    // MYT-317: SSO-aware. All string forms must hash identically.
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

        bool computePrimitiveProtocolEquals(value::Value& out,
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

        bool tryDispatchPrimitiveProtocolFast(
            ExecutionContext& context,
            vm::jit::ic::MethodProtocolFastKind kind,
            size_t argCount)
        {
            value::Value result;
            switch (kind)
            {
                case vm::jit::ic::MethodProtocolFastKind::PRIMITIVE_HASH_CODE:
                {
                    if (argCount != 0) return false;
                    value::Value receiver = context.stackManager->peek(0);
                    if (!computePrimitiveProtocolHash(result, receiver)) return false;
                    context.stackManager->pop();
                    context.stackManager->push(std::move(result));
                    return true;
                }
                case vm::jit::ic::MethodProtocolFastKind::PRIMITIVE_EQUALS:
                {
                    if (argCount != 1) return false;
                    value::Value receiver = context.stackManager->peek(1);
                    value::Value arg = context.stackManager->peek(0);
                    if (!computePrimitiveProtocolEquals(result, receiver, arg)) return false;
                    context.stackManager->pop();
                    context.stackManager->pop();
                    context.stackManager->push(std::move(result));
                    return true;
                }
                case vm::jit::ic::MethodProtocolFastKind::NONE:
                    return false;
            }
            return false;
        }

        vm::jit::ic::MethodProtocolFastKind classifyPrimitiveProtocolFastKind(
            const runtimeTypes::klass::ClassDefinition* classDef,
            const std::string& simpleMethodName,
            size_t argCount,
            const std::string& definingClassName)
        {
            (void)definingClassName;
            if (!classDef || value::classNameToPrimitiveTag(classDef->getName()) == value::PrimitiveTypeTag::NONE)
                return vm::jit::ic::MethodProtocolFastKind::NONE;
            if (simpleMethodName == "hashCode" && argCount == 0)
                return vm::jit::ic::MethodProtocolFastKind::PRIMITIVE_HASH_CODE;
            if (simpleMethodName == "equals" && argCount == 1)
                return vm::jit::ic::MethodProtocolFastKind::PRIMITIVE_EQUALS;
            return vm::jit::ic::MethodProtocolFastKind::NONE;
        }
    }

    bool InlineCacheExecutor::resolveICReceiverClass(
        const value::Value& receiver,
        const runtimeTypes::klass::ClassDefinition*& outClassDef,
        bool& outIsValueObject)
    {
        outClassDef = nullptr;
        outIsValueObject = false;
        // MYT-208: track classDef by pointer — covers OBJECT and STACK_OBJECT
        // receivers uniformly. Shape lookup is by ClassDefinition*, identical for both.
        if (value::isAnyObject(receiver))
        {
            auto* instance = value::asObjectInstanceRaw(receiver);
            outClassDef = instance ? instance->getClassDefinitionRaw() : nullptr;
        }
        else if (value::isValueObject(receiver))
        {
            // Value-class perf fix (Stage C): accept ValueObject receivers so
            // the IC populates at value-class sites. Otherwise OSR would fire
            // at a `p.sum()` site with the IC still UNINITIALIZED and the
            // speculative inliner would have nothing to consume.
            auto valueObj = value::asValueObject(receiver);
            if (valueObj)
            {
                outClassDef = valueObj->getClassDefinition().get();
                outIsValueObject = true;
            }
        }
        return outClassDef != nullptr;
    }

    bool InlineCacheExecutor::tryICDispatch(
        const bytecode::BytecodeProgram::Instruction& instr,
        vm::jit::ic::MethodInlineCache& cache,
        const runtimeTypes::klass::ClassDefinition* classDef,
        bool receiverIsValueObject,
        const value::Value& receiverValue,
        size_t argCount)
    {
        using namespace vm::jit::ic;

        // Phase 2c: MEGAMORPHIC also consults the cache — lookup transparently
        // searches the wide tier on top of the inline POLY array, so overflow
        // shapes still get a fast cached dispatch instead of falling straight
        // through to the runtime resolver.
        if (cache.state != ICState::MONOMORPHIC &&
            cache.state != ICState::POLYMORPHIC &&
            cache.state != ICState::MEGAMORPHIC)
        {
            return false;
        }

        const MethodICEntry* entry = cache.lookup(classDef);
        if (!entry || !entry->funcMetadata)
        {
            return false;
        }

        if (entry->protocolFastKind != MethodProtocolFastKind::NONE &&
            tryDispatchPrimitiveProtocolFast(context, entry->protocolFastKind, argCount))
        {
            return true;
        }

        auto* funcMeta = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(entry->funcMetadata);

        // Guard: native functions and entries without a real bytecode range
        // must not take the fast jump path (would underflow startOffset-1 or
        // jump into unrelated top-level bytecode). Fall through to the generic
        // handler, which dispatches native calls correctly.
        if (funcMeta->isNative || funcMeta->startOffset == 0)
        {
            objectExecutor->handleCallMethod(instr);
            return true;
        }

        // ValueObject IC hits take the generic handler's path —
        // invokeValueObjectMethod batch-materialises a temp ObjectInstance via
        // loadFromValueObject. This fires at most ~osrThreshold times per site
        // (500 today) before OSR tiers up, and dispatchDirectFromCachedTarget
        // is ObjectInstance-only by design. The critical bit of Stage C is
        // the populate path in slowMethodLookup — not this hit path.
        if (entry->receiverIsValueObject || receiverIsValueObject)
        {
            objectExecutor->handleCallMethod(instr);
            return true;
        }

        // MYT-173: shared with handleCallMethodCached — see the helper for the
        // full pop/push-frame/jump sequence.
        // MYT-197: intern on the callee's owning program. This path is
        // pre-promotion to CALL_METHOD_CACHED — only hot for a handful of
        // calls before tryPromoteToCached flips the opcode.
        dispatchDirectFromCachedTarget({
            funcMeta,
            entry->startOffset,
            entry->program,
            entry->programIndex,
            entry->program->internFrameName(entry->qualifiedName),
            entry->definingClassName,
            receiverValue,
            argCount
        });
        return true;
    }

    void InlineCacheExecutor::slowMethodLookup(
        const bytecode::BytecodeProgram::Instruction& instr,
        size_t icKey,
        const runtimeTypes::klass::ClassDefinition* classDef,
        bool receiverIsValueObject,
        size_t argCount)
    {
        using namespace vm::jit::ic;

        objectExecutor->handleCallMethod(instr);

        // Try to populate the cache for next time. Population is allowed for
        // every state — the wide tier absorbs entries past the inline POLY tier;
        // addEntry returns false only when the wide tier is full.
        const std::string& rawMethodName =
            context.program->getConstantPool().getString(instr.inlineOperands[0]);
        const std::string simpleMethodName =
            runtimeTypes::klass::SignatureUtils::extractSimpleName(rawMethodName);
        auto lookupResult =
            classDef->findInstanceMethodForCallNameCached(rawMethodName, argCount);
        if (!lookupResult.method) return;

        // MYT-182: resolve across main + loaded library programs so the IC entry
        // carries the owning program identity. Library callees would otherwise
        // dispatch against context.program and jump into unrelated bytecode.
        auto resolution = utils::MethodResolver::resolve(
            lookupResult.qualifiedName,
            lookupResult.definingClassName,
            simpleMethodName,
            context, *vm);
        if (!resolution.funcMetadata) return;

        MethodICEntry entry;
        entry.shape = classDef;
        entry.funcMetadata = resolution.funcMetadata;
        entry.startOffset = resolution.funcMetadata->startOffset;
        entry.qualifiedName = resolution.qualifiedName;
        // MYT-195: pre-resolve the class prefix once, here on the slow path, so
        // dispatchDirectFromCachedTarget can just copy the string into the
        // frame on every subsequent hit.
        size_t colonPos = entry.qualifiedName.find("::");
        if (colonPos != std::string::npos) {
            entry.definingClassName = entry.qualifiedName.substr(0, colonPos);
        }
        entry.program = resolution.program;
        entry.programIndex = resolution.programIndex;
        // Stage C: record the receiver kind so the JIT speculative inliner sees
        // the same IC data whether population happened here or in jit_call_method_ic.
        entry.receiverIsValueObject = receiverIsValueObject;
        entry.protocolFastKind = classifyPrimitiveProtocolFastKind(
            classDef, simpleMethodName, argCount,
            lookupResult.definingClassName);
        // MYT-315: cache the callee's JitFunction pointer if it's already
        // JIT-compiled. Null when the callee hasn't been JIT'd yet
        // (function-entry tiering / OSR may compile it later — a future IC
        // re-fill at this site will pick it up). Lookup is keyed by the
        // function's qualifiedName; OSR entries use "osr@<offset>" so this
        // can't accidentally return an OSR-entry pointer.
        if (vm) {
            if (auto* codeCache = vm->getJitCodeCache()) {
                entry.cachedJit = reinterpret_cast<const void*>(
                    codeCache->lookup(entry.qualifiedName));
            }
        }
        // MYT-183: re-fetch cache reference immediately before the write.
        // The reference captured at the top of handleCallMethodIC may have
        // been invalidated by nested CALL_METHODs that inserted into the same
        // methodCaches map and triggered rehash. Cheap: one hash lookup on
        // the slow path, which we already are on.
        MethodInlineCache& freshCache = icTable.getMethodIC(icKey);
        const ICState prevState = freshCache.state;
        bool added = freshCache.addEntry(entry);
        const bool transitionedToMega =
            prevState != ICState::MEGAMORPHIC &&
            freshCache.state == ICState::MEGAMORPHIC;

        // Phase 2c: with the wide tier, addEntry returns true for the 5th-and-
        // beyond shapes (they live in cache.wide). The POLY_CACHED fused opcode
        // still needs to be demoted on the POLY → MEGA transition, because its
        // dispatcher only walks the inline POLY array.
        if (transitionedToMega)
        {
            auto& mut = context.getMutableInstructionAt(icKey);
            if (mut.opcode == bytecode::OpCode::CALL_METHOD_POLY_CACHED ||
                mut.opcode == bytecode::OpCode::LOAD_LOCAL_CALL_POLY_CACHED)
            {
                if (mut.opcode == bytecode::OpCode::LOAD_LOCAL_CALL_POLY_CACHED)
                {
                    tryUnfusePair(bytecode::OpCode::CALL_METHOD_POLY_CACHED);
                    // Release-active invariant: assert is NDEBUG-stripped, but a
                    // tryUnfusePair regression that left the predecessor LOAD_LOCAL
                    // NOPed without demoting the fused opcode would silently break
                    // the next dispatch. Throw so the failure mode is loud in any build.
                    if (mut.opcode != bytecode::OpCode::CALL_METHOD_POLY_CACHED)
                    {
                        throw errors::RuntimeException(
                            "internal: tryUnfusePair must demote to CALL_METHOD_POLY_CACHED");
                    }
                }
                mut.opcode = bytecode::OpCode::CALL_METHOD;
                auto& cs = context.getOrCreateCachedState(icKey);
                cs.polyEntryCount = 0;
                if (cs.polyCachedDeoptCount < 255) ++cs.polyCachedDeoptCount;
            }
        }

        if (added)
        {
            // MYT-173: once the site stabilizes to MONO, promote the opcode so
            // subsequent dispatches skip the icTable hashmap probe and per-entry
            // scan entirely. The promote hooks gate on MONO/POLY state and are
            // no-ops when the cache is already MEGA.
            tryPromoteToCached(instr, entry);
            // MYT-203: once the site has 2-4 entries (POLY), snapshot all of
            // them on the side table and rewrite the opcode to CALL_METHOD_POLY_CACHED.
            // Idempotent — re-snapshots on 3rd / 4th shape arrivals at an
            // already-promoted site.
            tryPromoteToPolyCached(instr, entry);
        }
    }

    void InlineCacheExecutor::handleCallMethodIC(const bytecode::BytecodeProgram::Instruction& instr)
    {
        using namespace vm::jit::ic;

        if (objectExecutor && objectExecutor->tryDispatchSpecializedCollectionCall(instr))
        {
            return;
        }

        if (instr.numOperands() < 2)
        {
            objectExecutor->handleCallMethod(instr);
            return;
        }

        MethodInlineCache& cache = icTable.getMethodIC(context.instructionPointer);
        size_t argCount = instr.inlineOperands[1];

        // Stack layout: ... object arg0 arg1 ... argN-1. Object is at peek(argCount).
        if (context.stackManager->size() <= argCount)
        {
            objectExecutor->handleCallMethod(instr);
            return;
        }

        value::Value objectValue = context.stackManager->peek(argCount);

        const runtimeTypes::klass::ClassDefinition* classDef = nullptr;
        bool receiverIsValueObject = false;
        if (!resolveICReceiverClass(objectValue, classDef, receiverIsValueObject))
        {
            // Lambdas, primitives, nulls — delegate to the full handler.
            objectExecutor->handleCallMethod(instr);
            return;
        }

        if (tryICDispatch(instr, cache, classDef, receiverIsValueObject, objectValue, argCount))
        {
            return;
        }

        // IC miss — delegate + populate. Phase 2c: MEGAMORPHIC misses fall
        // through here too; addEntry routes the new shape into the wide tier
        // so the next call of the same shape hits the cached dispatch above.
        slowMethodLookup(instr, context.instructionPointer, classDef, receiverIsValueObject, argCount);
    }

    // dispatchDirectFromCachedTarget, tryPromoteToCached, deoptAndReprocess,
    // handleCallMethodCached live in InlineCacheExecutor_MethodCached.cpp.
}
