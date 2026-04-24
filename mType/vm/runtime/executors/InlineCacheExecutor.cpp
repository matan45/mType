#include "InlineCacheExecutor.hpp"
#include <cassert>
#include "ObjectExecutor.hpp"
#include "FunctionExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/NullCheckUtils.hpp"
#include "../utils/MethodResolver.hpp"
#include "../validation/AccessValidator.hpp"
#include "../../../runtimeTypes/klass/SignatureUtils.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../value/SmallArgsBuffer.hpp"
#include "../../../constants/SecurityConstants.hpp"

namespace vm::runtime
{
    InlineCacheExecutor::InlineCacheExecutor(ExecutionContext& ctx,
                                              vm::jit::ic::InlineCacheTable& table)
        : context(ctx)
        , icTable(table)
    {}

    void InlineCacheExecutor::handleGetFieldIC(const bytecode::BytecodeProgram::Instruction& instr)
    {
        using namespace vm::jit::ic;

        if (instr.operands.empty())
        {
            utils::ErrorLocationHelper::throwRuntimeError(context, "GET_FIELD requires operand");
        }

        FieldInlineCache& cache = icTable.getFieldIC(context.instructionPointer);

        value::Value objectValue = context.stackManager->pop();

        // Null check (skip if compiler guarantees non-null receiver)
        if (!(instr.flags & bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER))
        {
            if (utils::isNullValue(objectValue))
            {
                const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);
                utils::ErrorLocationHelper::throwError<errors::NullPointerException>(context,
                    "Cannot access field '" + fieldName + "' on null object");
            }
        }

        // Must be an object
        if (!value::isObject(objectValue))
        {
            // Fall back to generic path by pushing back and delegating
            context.stackManager->push(objectValue);
            objectExecutor->handleGetField(instr);
            return;
        }

        auto instance = value::asObject(objectValue);
        auto* classDef = instance->getClassDefinitionRaw();

        // IC fast path
        if (cache.state == ICState::MONOMORPHIC || cache.state == ICState::POLYMORPHIC)
        {
            const FieldICEntry* entry = cache.lookup(classDef);
            if (entry && entry->fieldIndex != SIZE_MAX)
            {
                // Ensure field vector is initialized
                if (!instance->hasFieldVector())
                {
                    instance->ensureFieldVector();
                }
                context.stackManager->push(instance->getFieldByIndex(entry->fieldIndex));
                return;
            }
        }

        // IC miss or uninitialized — validate access then cache
        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);

        auto fieldDef = instance->getField(fieldName);
        if (!fieldDef)
        {
            throw errors::FieldNotFoundException(fieldName, classDef->getName());
        }

        // Validate access control
        if (fieldDef->getAccessModifier() != ast::AccessModifier::PUBLIC)
        {
            auto fieldOwnerClass = instance->getClassDefinition()
                ->getFieldOwnerInHierarchy(fieldName, instance->getClassDefinition());
            std::string targetClassName = fieldOwnerClass
                ? fieldOwnerClass->getName() : classDef->getName();
            auto accessContext = objectExecutor->createAccessContext(targetClassName, false);
            validation::AccessValidator::validateFieldAccess(
                fieldName, fieldDef->getAccessModifier(), accessContext);
        }

        value::Value fieldValue = instance->getFieldValue(fieldName);
        context.stackManager->push(fieldValue);

        // Populate IC for non-static fields (access was validated above)
        if (cache.state != ICState::MEGAMORPHIC && !fieldDef->isStatic())
        {
            size_t fieldIndex = classDef->getFieldIndex(fieldName);
            if (fieldIndex != SIZE_MAX)
            {
                if (!instance->hasFieldVector())
                {
                    instance->ensureFieldVector();
                }
                cache.addEntry(classDef, fieldIndex);

                // MYT-194: once the site stabilizes to MONO, promote the opcode
                // so subsequent accesses skip the icTable hashmap probe and the
                // per-entry linear scan entirely. The helper internally gates
                // on cache.state == MONOMORPHIC, so POLY/MEGA transitions no-op.
                tryPromoteFieldToCached(instr, classDef, fieldIndex,
                                         bytecode::OpCode::GET_FIELD_CACHED);
            }
        }
    }

    void InlineCacheExecutor::handleSetFieldIC(const bytecode::BytecodeProgram::Instruction& instr)
    {
        using namespace vm::jit::ic;

        if (instr.operands.empty())
        {
            utils::ErrorLocationHelper::throwRuntimeError(context, "SET_FIELD requires operand");
        }

        FieldInlineCache& cache = icTable.getFieldIC(context.instructionPointer);

        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value newValue = context.stackManager->pop();
        value::Value objectValue = context.stackManager->pop();

        // Null check (skip if compiler guarantees non-null receiver)
        if (!(instr.flags & bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER))
        {
            if (utils::isNullValue(objectValue))
            {
                utils::ErrorLocationHelper::throwError<errors::NullPointerException>(context,
                    "Cannot set field '" + fieldName + "' on null object");
            }
        }

        if (!value::isObject(objectValue))
        {
            // Fall back: push values back and delegate to generic handler
            context.stackManager->push(objectValue);
            context.stackManager->push(newValue);
            objectExecutor->handleSetField(instr);
            return;
        }

        auto instance = value::asObject(objectValue);
        auto* classDef = instance->getClassDefinitionRaw();

        // IC fast path
        if (cache.state == ICState::MONOMORPHIC || cache.state == ICState::POLYMORPHIC)
        {
            const FieldICEntry* entry = cache.lookup(classDef);
            if (entry && entry->fieldIndex != SIZE_MAX)
            {
                if (!instance->hasFieldVector())
                {
                    instance->ensureFieldVector();
                }
                instance->setFieldByIndex(entry->fieldIndex, newValue);
                // Push newValue for assignment chaining
                context.stackManager->push(newValue);
                return;
            }
        }

        // IC miss — validate access then cache
        auto fieldDef = instance->getField(fieldName);

        // Validate access control
        if (fieldDef && fieldDef->getAccessModifier() != ast::AccessModifier::PUBLIC)
        {
            auto fieldOwnerClass = instance->getClassDefinition()
                ->getFieldOwnerInHierarchy(fieldName, instance->getClassDefinition());
            std::string targetClassName = fieldOwnerClass
                ? fieldOwnerClass->getName() : classDef->getName();
            auto accessContext = objectExecutor->createAccessContext(targetClassName, true);
            validation::AccessValidator::validateFieldAccess(
                fieldName, fieldDef->getAccessModifier(), accessContext);
        }

        instance->setField(fieldName, newValue);
        context.stackManager->push(newValue);

        // Populate IC for non-static fields (access was validated above)
        if (cache.state != ICState::MEGAMORPHIC && fieldDef && !fieldDef->isStatic())
        {
            size_t fieldIndex = classDef->getFieldIndex(fieldName);
            if (fieldIndex != SIZE_MAX)
            {
                if (!instance->hasFieldVector())
                {
                    instance->ensureFieldVector();
                }
                cache.addEntry(classDef, fieldIndex);

                // MYT-194: see handleGetFieldIC — promote once IC hits MONO.
                tryPromoteFieldToCached(instr, classDef, fieldIndex,
                                         bytecode::OpCode::SET_FIELD_CACHED);
            }
        }
    }

    void InlineCacheExecutor::handleInlineSetFieldIC(const bytecode::BytecodeProgram::Instruction& instr)
    {
        using namespace vm::jit::ic;

        FieldInlineCache& cache = icTable.getFieldIC(context.instructionPointer);

        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value newValue = context.stackManager->pop();
        value::Value objectValue = context.stackManager->pop();

        auto instance = value::asObject(objectValue);
        auto* classDef = instance->getClassDefinitionRaw();

        // IC fast path
        if (cache.state == ICState::MONOMORPHIC || cache.state == ICState::POLYMORPHIC)
        {
            const FieldICEntry* entry = cache.lookup(classDef);
            if (entry && entry->fieldIndex != SIZE_MAX)
            {
                if (!instance->hasFieldVector())
                {
                    instance->ensureFieldVector();
                }
                instance->setFieldByIndex(entry->fieldIndex, newValue);
                return;
            }
        }

        // IC miss — no access validation needed (trusted from inlined setter)
        instance->setField(fieldName, newValue);

        // Populate IC
        if (cache.state != ICState::MEGAMORPHIC)
        {
            auto fieldDef = instance->getField(fieldName);
            if (fieldDef && !fieldDef->isStatic())
            {
                size_t fieldIndex = classDef->getFieldIndex(fieldName);
                if (fieldIndex != SIZE_MAX)
                {
                    if (!instance->hasFieldVector())
                    {
                        instance->ensureFieldVector();
                    }
                    cache.addEntry(classDef, fieldIndex);
                }
            }
        }
    }

    void InlineCacheExecutor::handleInlineGetFieldIC(const bytecode::BytecodeProgram::Instruction& instr)
    {
        using namespace vm::jit::ic;

        const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);
        value::Value objectValue = context.stackManager->pop();

        // ValueObject and primitive receivers can't use IC — handle directly
        if (!value::isObject(objectValue))
        {
            if (value::isValueObject(objectValue)) {
                auto valueObj = value::asValueObject(objectValue);
                context.stackManager->push(valueObj->getFieldValue(fieldName));
            } else {
                // Re-push and delegate to the non-IC handler (which pops again)
                context.stackManager->push(objectValue);
                objectExecutor->handleInlineGetField(instr);
            }
            return;
        }

        auto instance = value::asObject(objectValue);
        auto* classDef = instance->getClassDefinitionRaw();

        FieldInlineCache& cache = icTable.getFieldIC(context.instructionPointer);

        // IC fast path
        if (cache.state == ICState::MONOMORPHIC || cache.state == ICState::POLYMORPHIC)
        {
            const FieldICEntry* entry = cache.lookup(classDef);
            if (entry && entry->fieldIndex != SIZE_MAX)
            {
                if (!instance->hasFieldVector())
                {
                    instance->ensureFieldVector();
                }
                context.stackManager->push(instance->getFieldByIndex(entry->fieldIndex));
                return;
            }
        }

        // IC miss
        context.stackManager->push(instance->getFieldValue(fieldName));

        // Populate IC
        if (cache.state != ICState::MEGAMORPHIC)
        {
            auto fieldDef = instance->getField(fieldName);
            if (fieldDef && !fieldDef->isStatic())
            {
                size_t fieldIndex = classDef->getFieldIndex(fieldName);
                if (fieldIndex != SIZE_MAX)
                {
                    if (!instance->hasFieldVector())
                    {
                        instance->ensureFieldVector();
                    }
                    cache.addEntry(classDef, fieldIndex);
                }
            }
        }
    }

    void InlineCacheExecutor::handleCallMethodIC(const bytecode::BytecodeProgram::Instruction& instr)
    {
        using namespace vm::jit::ic;

        if (instr.operands.size() < 2)
        {
            // Fall back to generic handler
            objectExecutor->handleCallMethod(instr);
            return;
        }

        MethodInlineCache& cache = icTable.getMethodIC(context.instructionPointer);

        // For method IC, we need to peek at the object on the stack (below args)
        size_t argCount = instr.operands[1];

        // The stack layout is: ... object arg0 arg1 ... argN-1
        // Object is at peek(argCount)
        if (context.stackManager->size() <= argCount)
        {
            objectExecutor->handleCallMethod(instr);
            return;
        }

        value::Value objectValue = context.stackManager->peek(argCount);

        // Value-class perf fix (Stage C): accept both ObjectInstance and
        // ValueObject receivers. Without this, handleCallMethod runs — which
        // never populates the IC for value-class sites — so by the time OSR
        // fires the IC at a `p.sum()` site is still UNINITIALIZED and the
        // speculative inliner has nothing to consume. Populating here lets
        // the OSR recompile inline the value-class method body.
        const runtimeTypes::klass::ClassDefinition* classDef = nullptr;
        bool receiverIsValueObject = false;
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance;
        if (value::isObject(objectValue))
        {
            instance = value::asObject(objectValue);
            classDef = instance->getClassDefinitionRaw();
        }
        else if (value::isValueObject(objectValue))
        {
            auto valueObj = value::asValueObject(objectValue);
            if (valueObj)
            {
                classDef = valueObj->getClassDefinition().get();
                receiverIsValueObject = true;
            }
        }

        if (!classDef)
        {
            // Lambdas, primitives, nulls — delegate to the full handler.
            objectExecutor->handleCallMethod(instr);
            return;
        }

        // IC fast path
        if (cache.state == ICState::MONOMORPHIC || cache.state == ICState::POLYMORPHIC)
        {
            const MethodICEntry* entry = cache.lookup(classDef);
            if (entry && entry->funcMetadata)
            {
                auto* funcMeta = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(entry->funcMetadata);

                // Guard: native functions and entries without a real bytecode
                // range must not take the fast jump path (would underflow
                // startOffset-1 or jump into unrelated top-level bytecode).
                // Fall through to the generic handler, which dispatches native
                // calls correctly.
                if (funcMeta->isNative || funcMeta->startOffset == 0)
                {
                    objectExecutor->handleCallMethod(instr);
                    return;
                }

                // ValueObject IC hits take the generic handler's path —
                // invokeValueObjectMethod batch-materialises a temp
                // ObjectInstance via loadFromValueObject. This fires at most
                // ~osrThreshold times per site (500 today) before OSR tiers
                // up, and dispatchDirectFromCachedTarget is ObjectInstance-
                // only by design. The critical bit of Stage C is the populate
                // path below — not this hit path.
                if (entry->receiverIsValueObject || receiverIsValueObject)
                {
                    objectExecutor->handleCallMethod(instr);
                    return;
                }

                // MYT-173: shared with handleCallMethodCached — see the helper
                // for the full pop/push-frame/jump sequence.
                // MYT-197: intern on the callee's owning program. This path
                // is pre-promotion to CALL_METHOD_CACHED — only hot for a
                // handful of calls before tryPromoteToCached flips the opcode;
                // the internFrameName hashmap hit is fine here.
                dispatchDirectFromCachedTarget(
                    funcMeta, entry->startOffset,
                    entry->program, entry->programIndex,
                    entry->program->internFrameName(entry->qualifiedName),
                    entry->definingClassName,
                    objectValue, argCount);
                return;
            }
        }

        // IC miss — delegate to full handler, then try to populate cache
        // We need to capture the function metadata after dispatch
        // For simplicity in V1, just delegate without populating on miss from this path
        if (cache.state == ICState::MEGAMORPHIC)
        {
            objectExecutor->handleCallMethod(instr);
            return;
        }

        // For UNINITIALIZED, let the full handler run and we'll populate on next hit
        // This is a tradeoff: we miss the first IC population opportunity,
        // but keep the code simple and correct.
        // Capture the caller's IP before handleCallMethod mutates it — this
        // is the bytecode offset that keys our cache entry.
        const size_t icKey = context.instructionPointer;
        objectExecutor->handleCallMethod(instr);

        // After the call, try to populate the cache for next time
        if (cache.state != ICState::MEGAMORPHIC)
        {
            const std::string& rawMethodName =
                context.program->getConstantPool().getString(instr.operands[0]);
            // MYT-181: strip class prefix + signature suffix. The compiler
            // emits mangled names ("Shape::compute/int") but
            // findInstanceMethodCached expects the simple name.
            const std::string simpleMethodName =
                runtimeTypes::klass::SignatureUtils::extractSimpleName(rawMethodName);
            auto lookupResult = classDef->findInstanceMethodCached(simpleMethodName, argCount);
            if (lookupResult.method)
            {
                // MYT-182: resolve across main + loaded library programs so
                // the IC entry carries the owning program identity. Library
                // callees would otherwise dispatch against context.program
                // and jump into unrelated bytecode.
                auto resolution = utils::MethodResolver::resolve(
                    lookupResult.qualifiedName,
                    lookupResult.definingClassName,
                    simpleMethodName,
                    context);
                if (resolution.funcMetadata)
                {
                    MethodICEntry entry;
                    entry.shape = classDef;
                    entry.funcMetadata = resolution.funcMetadata;
                    entry.startOffset = resolution.funcMetadata->startOffset;
                    entry.qualifiedName = resolution.qualifiedName;
                    // MYT-195: pre-resolve the class prefix once, here on the
                    // slow path, so dispatchDirectFromCachedTarget can just
                    // copy the string into the frame on every subsequent hit.
                    size_t colonPos = entry.qualifiedName.find("::");
                    if (colonPos != std::string::npos) {
                        entry.definingClassName = entry.qualifiedName.substr(0, colonPos);
                    }
                    entry.program = resolution.program;
                    entry.programIndex = resolution.programIndex;
                    // Stage C: record the receiver kind so the JIT speculative
                    // inliner sees the same IC data whether population happened
                    // here or in jit_call_method_ic.
                    entry.receiverIsValueObject = receiverIsValueObject;
                    // MYT-183: re-fetch cache reference immediately before
                    // the write. The reference captured at the top of this
                    // method may have been invalidated by nested CALL_METHODs
                    // that inserted into the same methodCaches map and
                    // triggered rehash. Cheap: one hash lookup on the slow
                    // path, which we already are on.
                    MethodInlineCache& freshCache = icTable.getMethodIC(icKey);
                    freshCache.addEntry(entry);

                    // MYT-173: once the site stabilizes to MONO, promote the
                    // opcode so subsequent dispatches skip the icTable hashmap
                    // probe and per-entry scan entirely. tryPromoteToCached
                    // itself skips value-class entries (CACHED fast-dispatch
                    // is ObjectInstance-only), so passing receiverIsValueObject
                    // through the entry is safe.
                    tryPromoteToCached(instr, entry);
                }
            }
        }
    }

    void InlineCacheExecutor::dispatchDirectFromCachedTarget(
        const bytecode::BytecodeProgram::FunctionMetadata* funcMeta,
        size_t startOffset,
        const bytecode::BytecodeProgram* program,
        size_t programIndex,
        bytecode::FunctionNameHandle qualifiedName,
        const std::string& definingClassName,
        value::Value objectValue,
        size_t argCount)
    {
        auto instance = value::asObject(objectValue);

        // Pop arguments into a small-buffer-optimized scratch buffer
        // (MYT-196: avoids per-call heap allocation on the MYT-173 hot path).
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i)
        {
            args[i - 1] = context.stackManager->pop();
        }
        // Pop the object (this)
        context.stackManager->pop();

        // Push call frame (with overflow protection).
        // MYT-197: 4-byte handle copy replaces std::string copy on the MYT-173
        // fast dispatch path.
        CallFrame frame;
        frame.returnAddress = context.instructionPointer;
        frame.functionName = qualifiedName;
        frame.localBase = context.stackManager->size();
        frame.frameBase = context.stackManager->size();
        frame.thisInstance = instance;
        // MYT-195: pre-resolved at IC populate / CACHED-promote time. Empty
        // when the qualified name carries no "::" prefix — same semantics as
        // the old per-call find/substr that left the field untouched.
        frame.definingClassName = definingClassName;
        // MYT-182: carry the callee's program identity on the frame so
        // ControlFlowExecutor restores context.program on return. If the
        // cached entry has no program recorded, fall back to the caller's.
        frame.programIndex = program
            ? programIndex
            : (context.callStack.empty() ? 0 : context.callStack.back().programIndex);
        context.pushCallFrame(frame);
        context.stats.functionCalls++;

        // MYT-182: switch context.program to the callee's owning program.
        if (program && program != context.program)
        {
            context.program = program;
        }

        // Push 'this' as first local
        context.stackManager->push(objectValue);

        // Push arguments as locals
        for (const auto& arg : args)
        {
            context.stackManager->push(arg);
        }

        // Reserve local variable slots beyond 'this' + arguments
        size_t pushedSlots = 1 + argCount;
        if (funcMeta->localCount > pushedSlots) {
            for (size_t i = 0; i < funcMeta->localCount - pushedSlots; ++i) {
                context.stackManager->push(std::monostate{});
            }
        }

        // Jump to function start (-1 because the dispatch loop increments)
        context.instructionPointer = startOffset - 1;
    }

    void InlineCacheExecutor::tryPromoteToCached(
        const bytecode::BytecodeProgram::Instruction& /*instr*/,
        const vm::jit::ic::MethodICEntry& entry)
    {
        using namespace vm::jit::ic;

        const size_t ip = context.instructionPointer;

        // Sticky demote — a site that has already deopted stays on the generic
        // CALL_METHOD path. Prevents flip/unflip ping-pong on unstable sites.
        // MYT-201: read via findCachedState so a never-promoted site doesn't
        // allocate an entry just to read the default 0.
        if (auto* existing = context.findCachedState(ip))
        {
            if (existing->cachedDeoptCount >= 1) return;
        }

        // ValueObject receivers use the MYT-167 temp-materialisation path in
        // jit_call_method; CACHED fast-dispatch is ObjectInstance-only for MVP.
        if (entry.receiverIsValueObject) return;

        auto* fm = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(entry.funcMetadata);
        if (!fm || fm->isNative || fm->startOffset == 0) return;

        // Only promote once the IC has settled to a single shape. POLY sites
        // are handled by a future CALL_METHOD_POLY_CACHED (out of MVP scope).
        MethodInlineCache& cache = icTable.getMethodIC(ip);
        if (cache.state != ICState::MONOMORPHIC) return;

        // MYT-201: writes go into the sparse side table.
        auto& state = context.getOrCreateCachedState(ip);
        state.cachedMethodShape        = entry.shape;
        state.cachedMethodFunc         = fm;
        state.cachedMethodProgram      = entry.program;
        state.cachedMethodProgramIndex = entry.programIndex;
        // MYT-197: intern on the callee's owning program (same program the
        // cached target lives in). Handle is copied 4 bytes at dispatch time.
        state.cachedMethodQualifiedName = entry.program->internFrameName(entry.qualifiedName);
        // MYT-195: snapshot the pre-resolved class prefix alongside qualifiedName.
        state.cachedMethodDefiningClassName = entry.definingClassName;

        auto& mut = context.getMutableInstructionAt(ip);
        mut.opcode = bytecode::OpCode::CALL_METHOD_CACHED;

        // MYT-198: opportunistically fuse with a preceding LOAD_LOCAL.
        tryFuseWithPriorLoadLocal(bytecode::OpCode::LOAD_LOCAL_CALL_CACHED);
    }

    void InlineCacheExecutor::deoptAndReprocess(
        const bytecode::BytecodeProgram::Instruction& /*instr*/)
    {
        const size_t ip = context.instructionPointer;
        auto& mut = context.getMutableInstructionAt(ip);
        // MYT-198: if the CACHED opcode was itself the second half of a fused
        // LOAD_LOCAL_CALL_CACHED pair, we must restore the NOPed LOAD_LOCAL
        // before demoting, otherwise the subsequent CALL_METHOD re-entry
        // would see a NOP predecessor and underflow on the receiver. The
        // fused handler already pushed the receiver onto the stack for this
        // dispatch, so the immediate re-entry below is fine; the un-fuse is
        // for all *future* dispatches of this IP.
        if (mut.opcode == bytecode::OpCode::LOAD_LOCAL_CALL_CACHED)
        {
            tryUnfusePair(bytecode::OpCode::CALL_METHOD_CACHED);
            // Fall through — mut.opcode is now CALL_METHOD_CACHED, demote to
            // CALL_METHOD as usual. Invariant-assert so a future change to
            // tryUnfusePair that leaves the opcode in a different state
            // surfaces loudly instead of silently breaking the demote.
            assert(mut.opcode == bytecode::OpCode::CALL_METHOD_CACHED &&
                   "tryUnfusePair must demote to CALL_METHOD_CACHED");
        }
        mut.opcode = bytecode::OpCode::CALL_METHOD;

        // MYT-201: clear cached fields via the side table. Entry is guaranteed
        // to exist here — a CACHED opcode implies a prior promote.
        auto& state = context.getOrCreateCachedState(ip);
        state.cachedMethodShape = nullptr;
        state.cachedMethodFunc = nullptr;
        state.cachedMethodProgram = nullptr;
        state.cachedMethodProgramIndex = 0;
        // MYT-197: integer sentinel replaces std::string::clear(). A re-promote
        // that happens to observe a stale handle value would index into the
        // interner and return an unrelated qualified name; INVALID_FN_HANDLE
        // prevents that.
        state.cachedMethodQualifiedName = bytecode::INVALID_FN_HANDLE;
        // MYT-195: clear the pre-resolved class prefix in lockstep with
        // qualifiedName so a subsequent re-promotion doesn't pick up a
        // stale class name from the prior shape.
        state.cachedMethodDefiningClassName.clear();
        if (state.cachedDeoptCount < 255) ++state.cachedDeoptCount;
        // Re-enter the generic IC path, which will observe the new shape
        // and (unless already MEGA) transition MONO -> POLY.
        handleCallMethodIC(mut);
    }

    void InlineCacheExecutor::handleCallMethodCached(
        const bytecode::BytecodeProgram::Instruction& instr,
        const bytecode::BytecodeProgram::CachedInstructionState& state)
    {
        if (instr.operands.size() < 2 || !state.cachedMethodFunc || !state.cachedMethodShape)
        {
            deoptAndReprocess(instr);
            return;
        }

        size_t argCount = instr.operands[1];
        if (context.stackManager->size() <= argCount)
        {
            deoptAndReprocess(instr);
            return;
        }

        value::Value objectValue = context.stackManager->peek(argCount);
        if (!value::isObject(objectValue))
        {
            deoptAndReprocess(instr);
            return;
        }

        auto instance = value::asObject(objectValue);
        auto* shape = instance->getClassDefinitionRaw();
        if (shape != state.cachedMethodShape)
        {
            deoptAndReprocess(instr);
            return;
        }

        // Hot path: shape matched — dispatch directly from the embedded target.
        // No icTable.getMethodIC probe, no per-entry scan.
        dispatchDirectFromCachedTarget(
            state.cachedMethodFunc,
            state.cachedMethodFunc->startOffset,
            state.cachedMethodProgram,
            state.cachedMethodProgramIndex,
            state.cachedMethodQualifiedName,
            state.cachedMethodDefiningClassName,
            objectValue, argCount);
    }

    void InlineCacheExecutor::tryPromoteFieldToCached(
        const bytecode::BytecodeProgram::Instruction& /*instr*/,
        const runtimeTypes::klass::ClassDefinition* shape,
        size_t fieldIndex,
        bytecode::OpCode cachedOp)
    {
        using namespace vm::jit::ic;

        const size_t ip = context.instructionPointer;

        // Sticky demote — a site that already deopted once stays on the generic
        // path. MYT-201: read via findCachedState so a never-promoted site
        // doesn't allocate an entry just to check the default 0.
        if (auto* existing = context.findCachedState(ip))
        {
            if (existing->cachedFieldDeoptCount >= 1) return;
        }

        if (!shape || fieldIndex == SIZE_MAX) return;

        // Only promote once the IC has settled to a single shape. POLY sites
        // are a future CACHED_POLY variant (out of MVP scope).
        FieldInlineCache& cache = icTable.getFieldIC(ip);
        if (cache.state != ICState::MONOMORPHIC) return;

        auto& state = context.getOrCreateCachedState(ip);
        state.cachedFieldShape = shape;
        state.cachedFieldIndex = fieldIndex;

        auto& mut = context.getMutableInstructionAt(ip);
        mut.opcode = cachedOp;

        // MYT-198: only GET_FIELD_CACHED participates in fusion. SET_FIELD_CACHED
        // pops both value + receiver, and a preceding LOAD_LOCAL at IP-1 would
        // be the receiver — but the "value" has already been pushed by whatever
        // sits at IP-2, so a two-instruction fusion can't cleanly fold the SET.
        // Keep SET_FIELD_CACHED un-fused for MVP.
        if (cachedOp == bytecode::OpCode::GET_FIELD_CACHED) {
            tryFuseWithPriorLoadLocal(bytecode::OpCode::LOAD_LOCAL_GET_FIELD_CACHED);
        }
    }

    void InlineCacheExecutor::deoptGetFieldAndReprocess(
        const bytecode::BytecodeProgram::Instruction& /*instr*/)
    {
        const size_t ip = context.instructionPointer;
        auto& mut = context.getMutableInstructionAt(ip);
        // MYT-198: see deoptAndReprocess — same un-fuse-before-demote story.
        if (mut.opcode == bytecode::OpCode::LOAD_LOCAL_GET_FIELD_CACHED)
        {
            tryUnfusePair(bytecode::OpCode::GET_FIELD_CACHED);
            assert(mut.opcode == bytecode::OpCode::GET_FIELD_CACHED &&
                   "tryUnfusePair must demote to GET_FIELD_CACHED");
        }
        mut.opcode = bytecode::OpCode::GET_FIELD;

        auto& state = context.getOrCreateCachedState(ip);
        state.cachedFieldShape = nullptr;
        state.cachedFieldIndex = SIZE_MAX;
        if (state.cachedFieldDeoptCount < 255) ++state.cachedFieldDeoptCount;
        // Re-enter the generic IC path so the new shape is observed.
        handleGetFieldIC(mut);
    }

    void InlineCacheExecutor::deoptSetFieldAndReprocess(
        const bytecode::BytecodeProgram::Instruction& /*instr*/)
    {
        const size_t ip = context.instructionPointer;
        auto& mut = context.getMutableInstructionAt(ip);
        mut.opcode = bytecode::OpCode::SET_FIELD;

        auto& state = context.getOrCreateCachedState(ip);
        state.cachedFieldShape = nullptr;
        state.cachedFieldIndex = SIZE_MAX;
        if (state.cachedFieldDeoptCount < 255) ++state.cachedFieldDeoptCount;
        handleSetFieldIC(mut);
    }

    void InlineCacheExecutor::handleGetFieldCached(
        const bytecode::BytecodeProgram::Instruction& instr,
        const bytecode::BytecodeProgram::CachedInstructionState& state)
    {
        if (instr.operands.empty() || !state.cachedFieldShape
            || state.cachedFieldIndex == SIZE_MAX)
        {
            deoptGetFieldAndReprocess(instr);
            return;
        }
        if (context.stackManager->size() < 1)
        {
            deoptGetFieldAndReprocess(instr);
            return;
        }

        // Peek so that a deopt leaves the stack untouched — handleGetFieldIC
        // pops the receiver itself on re-entry. Matches handleCallMethodCached.
        value::Value objectValue = context.stackManager->peek(0);

        // Null check (respect the compiler's NONNULL_RECEIVER flag). Match
        // handleGetFieldIC's throw-on-null behavior rather than deopting —
        // null at a previously-non-null site is a program error, not a
        // type-feedback failure.
        if (!(instr.flags & bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER))
        {
            if (utils::isNullValue(objectValue))
            {
                const std::string& fieldName =
                    context.program->getConstantPool().getString(instr.operands[0]);
                utils::ErrorLocationHelper::throwError<errors::NullPointerException>(
                    context,
                    "Cannot access field '" + fieldName + "' on null object");
            }
        }

        if (!value::isObject(objectValue))
        {
            deoptGetFieldAndReprocess(instr);
            return;
        }

        auto instance = value::asObject(objectValue);
        auto* shape = instance->getClassDefinitionRaw();
        if (shape != state.cachedFieldShape)
        {
            deoptGetFieldAndReprocess(instr);
            return;
        }

        // Hot path: shape matched — indexed read, no IC probe, no access check.
        // Access validation was done at promote time; shape identity means the
        // validated field layout still applies.
        if (!instance->hasFieldVector())
        {
            instance->ensureFieldVector();
        }
        value::Value fieldValue = instance->getFieldByIndex(state.cachedFieldIndex);
        context.stackManager->pop();
        context.stackManager->push(fieldValue);
    }

    void InlineCacheExecutor::handleSetFieldCached(
        const bytecode::BytecodeProgram::Instruction& instr,
        const bytecode::BytecodeProgram::CachedInstructionState& state)
    {
        if (instr.operands.empty() || !state.cachedFieldShape
            || state.cachedFieldIndex == SIZE_MAX)
        {
            deoptSetFieldAndReprocess(instr);
            return;
        }
        if (context.stackManager->size() < 2)
        {
            deoptSetFieldAndReprocess(instr);
            return;
        }

        // Stack: [..., object, newValue]. peek(0)=newValue, peek(1)=object.
        value::Value newValue = context.stackManager->peek(0);
        value::Value objectValue = context.stackManager->peek(1);

        if (!(instr.flags & bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER))
        {
            if (utils::isNullValue(objectValue))
            {
                const std::string& fieldName =
                    context.program->getConstantPool().getString(instr.operands[0]);
                utils::ErrorLocationHelper::throwError<errors::NullPointerException>(
                    context,
                    "Cannot set field '" + fieldName + "' on null object");
            }
        }

        if (!value::isObject(objectValue))
        {
            deoptSetFieldAndReprocess(instr);
            return;
        }

        auto instance = value::asObject(objectValue);
        auto* shape = instance->getClassDefinitionRaw();
        if (shape != state.cachedFieldShape)
        {
            deoptSetFieldAndReprocess(instr);
            return;
        }

        if (!instance->hasFieldVector())
        {
            instance->ensureFieldVector();
        }
        instance->setFieldByIndex(state.cachedFieldIndex, newValue);
        // Match handleSetFieldIC: consume both operands, push newValue for
        // assignment chaining.
        context.stackManager->pop();
        context.stackManager->pop();
        context.stackManager->push(newValue);
    }

    void InlineCacheExecutor::tryFuseWithPriorLoadLocal(bytecode::OpCode fusedOp)
    {
        const size_t ip = context.instructionPointer;
        if (ip == 0) return;  // No predecessor slot.

        // Prior must be LOAD_LOCAL within the same program. loadedPrograms
        // routing isn't relevant — IP and program are already coherent for
        // the currently-executing frame.
        const auto& prev = context.program->getInstruction(ip - 1);
        if (prev.opcode != bytecode::OpCode::LOAD_LOCAL) return;
        if (prev.operands.empty()) return;

        // Current IP must not be reachable via any control-flow path other
        // than falling through from IP-1. Otherwise a jump landing directly
        // on the fused op would execute LOAD_LOCAL(fusedSlot) on a stack
        // that the jumping caller didn't set up for it.
        if (context.program->isFusionUnsafeTarget(ip)) return;

        // MYT-201: fusion reads/writes live on the side-table entry. Current
        // IP's entry is guaranteed to exist (caller just promoted to CACHED).
        auto& state = context.getOrCreateCachedState(ip);

        // Sticky un-fuse — a site that already had its fusion rolled back
        // should not be re-fused. Mirrors cachedDeoptCount's sticky semantics.
        if (state.fusedDeoptCount >= 1) return;

        // Lambda / shared-frame guard. LOAD_LOCAL inside a lambda frame has
        // captured-variable semantics that the fused executor doesn't
        // replicate (it reads from frameBase + slot directly). Skipping here
        // is safer than duplicating VariableExecutor::handleLoadLocal's
        // capture logic in every fused op; lambda-body fusion is follow-up.
        if (!context.callStack.empty())
        {
            const auto& frame = context.callStack.back();
            if (frame.originatingLambda || frame.sharedFrame) return;
        }

        // MYT-173 precedent: an in-place opcode rewrite (prev→NOP, current→
        // fused) keeps the instruction vector length stable, so no jump
        // offsets need fixing up. fusedSlot captures what used to be the
        // LOAD_LOCAL's operand[0].
        uint64_t slot = prev.operands[0];
        // fusedSlot is uint32_t — assert before truncation. Slot indices are
        // capped by constants::security::MAX_LOCAL_STACK_PER_FRAME at every
        // LOAD_LOCAL / STORE_LOCAL entry, so this should never fire in a
        // well-formed program, but an oversized operand would otherwise
        // silently alias a different slot at the fused site.
        assert(slot <= UINT32_MAX &&
               "LOAD_LOCAL fusion: slot index exceeds fusedSlot width");
        auto& prevMut = context.getMutableInstructionAt(ip - 1);
        prevMut.opcode = bytecode::OpCode::NOP;
        prevMut.operands.clear();

        state.fusedSlot = static_cast<uint32_t>(slot);
        auto& mut = context.getMutableInstructionAt(ip);
        mut.opcode = fusedOp;
    }

    void InlineCacheExecutor::handleLoadLocalCallCached(
        const bytecode::BytecodeProgram::Instruction& instr,
        const bytecode::BytecodeProgram::CachedInstructionState& state)
    {
        // Fast replay of the NOPed LOAD_LOCAL: the fusion trigger guaranteed
        // a non-lambda, non-shared frame, so the simple localBase + slot read
        // is sufficient — we don't need to replicate handleLoadLocal's
        // capture-chain logic. SECURITY: same MAX_LOCAL_STACK_PER_FRAME cap
        // as handleLoadLocal, on fusedSlot.
        size_t slot = static_cast<size_t>(state.fusedSlot);
        if (slot >= constants::security::MAX_LOCAL_STACK_PER_FRAME)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "LOAD_LOCAL_CALL_CACHED slot index " + std::to_string(slot) +
                " exceeds per-frame limit");
        }

        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        size_t stackPos = frameBase + slot;
        if (stackPos >= context.stackManager->size())
        {
            // Stack shape doesn't match the fusion expectation — un-fuse and
            // re-dispatch through the generic IC path. tryUnfusePair bumps
            // fusedDeoptCount so we don't re-fuse.
            tryUnfusePair(bytecode::OpCode::CALL_METHOD_CACHED);
            // Receiver isn't on stack; replay through handleCallMethodIC
            // which will fail with the correct error at the generic site.
            handleCallMethodIC(
                context.getMutableInstructionAt(context.instructionPointer));
            return;
        }
        context.stackManager->push((*context.stackManager)[stackPos]);

        // Receiver now on stack; delegate to the standard CACHED handler.
        // Same state entry — no second side-table lookup needed.
        handleCallMethodCached(instr, state);
    }

    void InlineCacheExecutor::handleLoadLocalGetFieldCached(
        const bytecode::BytecodeProgram::Instruction& instr,
        const bytecode::BytecodeProgram::CachedInstructionState& state)
    {
        size_t slot = static_cast<size_t>(state.fusedSlot);
        if (slot >= constants::security::MAX_LOCAL_STACK_PER_FRAME)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context,
                "LOAD_LOCAL_GET_FIELD_CACHED slot index " + std::to_string(slot) +
                " exceeds per-frame limit");
        }

        size_t frameBase = context.callStack.empty() ? 0 : context.callStack.back().localBase;
        size_t stackPos = frameBase + slot;
        if (stackPos >= context.stackManager->size())
        {
            tryUnfusePair(bytecode::OpCode::GET_FIELD_CACHED);
            handleGetFieldIC(
                context.getMutableInstructionAt(context.instructionPointer));
            return;
        }
        context.stackManager->push((*context.stackManager)[stackPos]);

        handleGetFieldCached(instr, state);
    }

    bool InlineCacheExecutor::tryUnfusePair(bytecode::OpCode underlyingCached)
    {
        const size_t ip = context.instructionPointer;
        if (ip == 0) return false;

        auto& mut = context.getMutableInstructionAt(ip);
        if (mut.opcode != bytecode::OpCode::LOAD_LOCAL_CALL_CACHED &&
            mut.opcode != bytecode::OpCode::LOAD_LOCAL_GET_FIELD_CACHED)
        {
            return false;
        }

        // MYT-201: fused state lives in the side table. Entry is guaranteed
        // to exist (this IP was fused → promoted → an entry was created).
        auto& state = context.getOrCreateCachedState(ip);

        // Restore LOAD_LOCAL at IP-1 from the captured fusedSlot.
        auto& prevMut = context.getMutableInstructionAt(ip - 1);
        prevMut.opcode = bytecode::OpCode::LOAD_LOCAL;
        prevMut.operands = { static_cast<uint64_t>(state.fusedSlot) };

        // Demote current back to the underlying CACHED opcode and make the
        // un-fuse sticky so the next promotion here won't re-fuse.
        mut.opcode = underlyingCached;
        if (state.fusedDeoptCount < 255) ++state.fusedDeoptCount;
        return true;
    }
}
