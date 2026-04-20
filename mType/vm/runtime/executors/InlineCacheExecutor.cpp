#include "InlineCacheExecutor.hpp"
#include "ObjectExecutor.hpp"
#include "FunctionExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/NullCheckUtils.hpp"
#include "../utils/MethodResolver.hpp"
#include "../validation/AccessValidator.hpp"
#include "../../../runtimeTypes/klass/SignatureUtils.hpp"
#include "../../../value/ValueShim.hpp"

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
        auto* classDef = instance->getClassDefinition().get();

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
        auto* classDef = instance->getClassDefinition().get();

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
        auto* classDef = instance->getClassDefinition().get();

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
        auto* classDef = instance->getClassDefinition().get();

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

        if (!value::isObject(objectValue))
        {
            // Not a simple object — delegate to full handler (handles lambdas, etc.)
            objectExecutor->handleCallMethod(instr);
            return;
        }

        auto instance = value::asObject(objectValue);
        auto* classDef = instance->getClassDefinition().get();

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

                // MYT-173: shared with handleCallMethodCached — see the helper
                // for the full pop/push-frame/jump sequence.
                dispatchDirectFromCachedTarget(
                    funcMeta, entry->startOffset,
                    entry->program, entry->programIndex,
                    entry->qualifiedName, entry->definingClassName,
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
                    // probe and per-entry scan entirely.
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
        const std::string& qualifiedName,
        const std::string& definingClassName,
        value::Value objectValue,
        size_t argCount)
    {
        auto instance = value::asObject(objectValue);

        // Pop arguments
        std::vector<value::Value> args(argCount);
        for (size_t i = argCount; i > 0; --i)
        {
            args[i - 1] = context.stackManager->pop();
        }
        // Pop the object (this)
        context.stackManager->pop();

        // Push call frame (with overflow protection)
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
        const bytecode::BytecodeProgram::Instruction& instr,
        const vm::jit::ic::MethodICEntry& entry)
    {
        using namespace vm::jit::ic;

        // Sticky demote — a site that has already deopted stays on the generic
        // CALL_METHOD path. Prevents flip/unflip ping-pong on unstable sites.
        if (instr.cachedDeoptCount >= 1) return;

        // ValueObject receivers use the MYT-167 temp-materialisation path in
        // jit_call_method; CACHED fast-dispatch is ObjectInstance-only for MVP.
        if (entry.receiverIsValueObject) return;

        auto* fm = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(entry.funcMetadata);
        if (!fm || fm->isNative || fm->startOffset == 0) return;

        // Only promote once the IC has settled to a single shape. POLY sites
        // are handled by a future CALL_METHOD_POLY_CACHED (out of MVP scope).
        MethodInlineCache& cache = icTable.getMethodIC(context.instructionPointer);
        if (cache.state != ICState::MONOMORPHIC) return;

        auto& mut = context.getMutableInstructionAt(context.instructionPointer);
        mut.cachedMethodShape        = entry.shape;
        mut.cachedMethodFunc         = fm;
        mut.cachedMethodProgram      = entry.program;
        mut.cachedMethodProgramIndex = entry.programIndex;
        mut.cachedMethodQualifiedName = entry.qualifiedName;
        // MYT-195: snapshot the pre-resolved class prefix alongside qualifiedName.
        mut.cachedMethodDefiningClassName = entry.definingClassName;
        mut.opcode = bytecode::OpCode::CALL_METHOD_CACHED;
    }

    void InlineCacheExecutor::deoptAndReprocess(
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        auto& mut = context.getMutableInstructionAt(context.instructionPointer);
        mut.opcode = bytecode::OpCode::CALL_METHOD;
        mut.cachedMethodShape = nullptr;
        mut.cachedMethodFunc = nullptr;
        mut.cachedMethodProgram = nullptr;
        mut.cachedMethodProgramIndex = 0;
        mut.cachedMethodQualifiedName.clear();
        // MYT-195: clear the pre-resolved class prefix in lockstep with
        // qualifiedName so a subsequent re-promotion doesn't pick up a
        // stale class name from the prior shape.
        mut.cachedMethodDefiningClassName.clear();
        if (mut.cachedDeoptCount < 255) ++mut.cachedDeoptCount;
        // `instr` and `mut` alias the same slot; single-threaded VM, so this is
        // safe. Re-enter the generic IC path, which will observe the new shape
        // and (unless already MEGA) transition MONO -> POLY.
        handleCallMethodIC(mut);
    }

    void InlineCacheExecutor::handleCallMethodCached(
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (instr.operands.size() < 2 || !instr.cachedMethodFunc || !instr.cachedMethodShape)
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
        auto* shape = instance->getClassDefinition().get();
        if (shape != instr.cachedMethodShape)
        {
            deoptAndReprocess(instr);
            return;
        }

        // Hot path: shape matched — dispatch directly from the embedded target.
        // No icTable.getMethodIC probe, no per-entry scan.
        dispatchDirectFromCachedTarget(
            instr.cachedMethodFunc,
            instr.cachedMethodFunc->startOffset,
            instr.cachedMethodProgram,
            instr.cachedMethodProgramIndex,
            instr.cachedMethodQualifiedName,
            instr.cachedMethodDefiningClassName,
            objectValue, argCount);
    }

    void InlineCacheExecutor::tryPromoteFieldToCached(
        const bytecode::BytecodeProgram::Instruction& /*instr*/,
        const runtimeTypes::klass::ClassDefinition* shape,
        size_t fieldIndex,
        bytecode::OpCode cachedOp)
    {
        using namespace vm::jit::ic;

        // Sticky demote — a site that already deopted once stays on the generic
        // path. Read through the mutable instruction since `instr` is an alias
        // of the same slot and we're about to mutate it anyway.
        auto& mut = context.getMutableInstructionAt(context.instructionPointer);
        if (mut.cachedFieldDeoptCount >= 1) return;

        if (!shape || fieldIndex == SIZE_MAX) return;

        // Only promote once the IC has settled to a single shape. POLY sites
        // are a future CACHED_POLY variant (out of MVP scope).
        FieldInlineCache& cache = icTable.getFieldIC(context.instructionPointer);
        if (cache.state != ICState::MONOMORPHIC) return;

        mut.cachedFieldShape = shape;
        mut.cachedFieldIndex = fieldIndex;
        mut.opcode = cachedOp;
    }

    void InlineCacheExecutor::deoptGetFieldAndReprocess(
        const bytecode::BytecodeProgram::Instruction& /*instr*/)
    {
        auto& mut = context.getMutableInstructionAt(context.instructionPointer);
        mut.opcode = bytecode::OpCode::GET_FIELD;
        mut.cachedFieldShape = nullptr;
        mut.cachedFieldIndex = SIZE_MAX;
        if (mut.cachedFieldDeoptCount < 255) ++mut.cachedFieldDeoptCount;
        // Re-enter the generic IC path so the new shape is observed.
        handleGetFieldIC(mut);
    }

    void InlineCacheExecutor::deoptSetFieldAndReprocess(
        const bytecode::BytecodeProgram::Instruction& /*instr*/)
    {
        auto& mut = context.getMutableInstructionAt(context.instructionPointer);
        mut.opcode = bytecode::OpCode::SET_FIELD;
        mut.cachedFieldShape = nullptr;
        mut.cachedFieldIndex = SIZE_MAX;
        if (mut.cachedFieldDeoptCount < 255) ++mut.cachedFieldDeoptCount;
        handleSetFieldIC(mut);
    }

    void InlineCacheExecutor::handleGetFieldCached(
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (instr.operands.empty() || !instr.cachedFieldShape
            || instr.cachedFieldIndex == SIZE_MAX)
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
        auto* shape = instance->getClassDefinition().get();
        if (shape != instr.cachedFieldShape)
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
        value::Value fieldValue = instance->getFieldByIndex(instr.cachedFieldIndex);
        context.stackManager->pop();
        context.stackManager->push(fieldValue);
    }

    void InlineCacheExecutor::handleSetFieldCached(
        const bytecode::BytecodeProgram::Instruction& instr)
    {
        if (instr.operands.empty() || !instr.cachedFieldShape
            || instr.cachedFieldIndex == SIZE_MAX)
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
        auto* shape = instance->getClassDefinition().get();
        if (shape != instr.cachedFieldShape)
        {
            deoptSetFieldAndReprocess(instr);
            return;
        }

        if (!instance->hasFieldVector())
        {
            instance->ensureFieldVector();
        }
        instance->setFieldByIndex(instr.cachedFieldIndex, newValue);
        // Match handleSetFieldIC: consume both operands, push newValue for
        // assignment chaining.
        context.stackManager->pop();
        context.stackManager->pop();
        context.stackManager->push(newValue);
    }
}
