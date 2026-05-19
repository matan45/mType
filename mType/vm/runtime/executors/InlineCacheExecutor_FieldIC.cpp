#include "InlineCacheExecutor.hpp"
#include "ObjectExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/NullCheckUtils.hpp"
#include "../validation/AccessValidator.hpp"

namespace vm::runtime
{
    void InlineCacheExecutor::handleGetFieldIC(const bytecode::BytecodeProgram::Instruction& instr)
    {
        using namespace vm::jit::ic;

        if (instr.numOperands() == 0)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context, "GET_FIELD requires operand");
        }

        FieldInlineCache& cache = icTable.getFieldIC(context.instructionPointer);

        value::Value objectValue = context.stackManager->pop();

        // Null check (skip if compiler guarantees non-null receiver).
        if (!(instr.flags & bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER))
        {
            if (utils::isNullValue(objectValue))
            {
                const std::string& fieldName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
                utils::ErrorLocationHelper::throwError<errors::NullPointerException>(context,
                    "Cannot access field '" + fieldName + "' on null object");
            }
        }

        // Must be an object (OBJECT or STACK_OBJECT — MYT-208).
        if (!value::isAnyObject(objectValue))
        {
            // Fall back to generic path by pushing back and delegating.
            context.stackManager->push(objectValue);
            objectExecutor->handleGetField(instr);
            return;
        }

        // MYT-208: raw unwrap covers both heap (OBJECT) and stack-promoted
        // (STACK_OBJECT) receivers; the IC shape key is the ClassDefinition
        // pointer, which lives at the same offset on the ObjectInstance
        // regardless of how we got the raw pointer.
        auto* instance = value::asObjectInstanceRaw(objectValue);
        auto* classDef = instance->getClassDefinitionRaw();

        // IC fast path.
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

        // IC miss or uninitialized — validate access then cache.
        const std::string& fieldName = context.program->getConstantPool().getString(instr.inlineOperands[0]);

        auto fieldDef = instance->getField(fieldName);
        if (!fieldDef)
        {
            throw errors::FieldNotFoundException(fieldName, classDef->getName());
        }

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

        // Populate IC for non-static fields (access was validated above).
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
                // per-entry linear scan entirely. The helper internally gates on
                // cache.state == MONOMORPHIC, so POLY/MEGA transitions no-op.
                tryPromoteFieldToCached(instr, classDef, fieldIndex,
                                         bytecode::OpCode::GET_FIELD_CACHED);
            }
        }
    }

    void InlineCacheExecutor::handleSetFieldIC(const bytecode::BytecodeProgram::Instruction& instr)
    {
        using namespace vm::jit::ic;

        if (instr.numOperands() == 0)
        {
            utils::ErrorLocationHelper::throwRuntimeError(context, "SET_FIELD requires operand");
        }

        FieldInlineCache& cache = icTable.getFieldIC(context.instructionPointer);

        const std::string& fieldName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        value::Value newValue = context.stackManager->pop();
        value::Value objectValue = context.stackManager->pop();

        if (!(instr.flags & bytecode::BytecodeProgram::INSTR_FLAG_NONNULL_RECEIVER))
        {
            if (utils::isNullValue(objectValue))
            {
                utils::ErrorLocationHelper::throwError<errors::NullPointerException>(context,
                    "Cannot set field '" + fieldName + "' on null object");
            }
        }

        // MYT-208: accept STACK_OBJECT alongside OBJECT.
        if (!value::isAnyObject(objectValue))
        {
            context.stackManager->push(objectValue);
            context.stackManager->push(newValue);
            objectExecutor->handleSetField(instr);
            return;
        }

        auto* instance = value::asObjectInstanceRaw(objectValue);
        auto* classDef = instance->getClassDefinitionRaw();

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
                // Push newValue for assignment chaining.
                context.stackManager->push(newValue);
                return;
            }
        }

        // IC miss — validate access then cache.
        auto fieldDef = instance->getField(fieldName);

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

        const std::string& fieldName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        value::Value newValue = context.stackManager->pop();
        value::Value objectValue = context.stackManager->pop();

        // MYT-208: accept STACK_OBJECT alongside OBJECT.
        auto* instance = value::asObjectInstanceRaw(objectValue);
        auto* classDef = instance->getClassDefinitionRaw();

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

        // IC miss — no access validation needed (trusted from inlined setter).
        instance->setField(fieldName, newValue);

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

        const std::string& fieldName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        value::Value objectValue = context.stackManager->pop();

        // ValueObject and primitive receivers can't use IC — handle directly.
        // MYT-208: accept STACK_OBJECT (raw borrowed) alongside OBJECT.
        if (!value::isAnyObject(objectValue))
        {
            if (value::isValueObject(objectValue)) {
                auto valueObj = value::asValueObject(objectValue);
                context.stackManager->push(valueObj->getFieldValue(fieldName));
            } else {
                // Re-push and delegate to the non-IC handler (which pops again).
                context.stackManager->push(objectValue);
                objectExecutor->handleInlineGetField(instr);
            }
            return;
        }

        auto* instance = value::asObjectInstanceRaw(objectValue);
        auto* classDef = instance->getClassDefinitionRaw();

        FieldInlineCache& cache = icTable.getFieldIC(context.instructionPointer);

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

        context.stackManager->push(instance->getFieldValue(fieldName));

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

    void InlineCacheExecutor::tryPromoteFieldToCached(
        const bytecode::BytecodeProgram::Instruction& /*instr*/,
        const runtimeTypes::klass::ClassDefinition* shape,
        size_t fieldIndex,
        bytecode::OpCode cachedOp)
    {
        using namespace vm::jit::ic;

        const size_t ip = context.instructionPointer;

        // MYT-202: only promote when the IP actually holds the generic
        // GET_FIELD / SET_FIELD we're specializing. handleGetFieldIC /
        // handleSetFieldIC can be dispatched as shims by fused-opcode handlers
        // (LOAD_GET_FIELD) — rewriting the fused opcode to CACHED would
        // destroy the fusion.
        const auto currentOp = context.getMutableInstructionAt(ip).opcode;
        if (cachedOp == bytecode::OpCode::GET_FIELD_CACHED &&
            currentOp != bytecode::OpCode::GET_FIELD) return;
        if (cachedOp == bytecode::OpCode::SET_FIELD_CACHED &&
            currentOp != bytecode::OpCode::SET_FIELD) return;

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
            // Release-active invariant — see deoptAndReprocess for rationale.
            if (mut.opcode != bytecode::OpCode::GET_FIELD_CACHED)
            {
                throw errors::RuntimeException(
                    "internal: tryUnfusePair must demote to GET_FIELD_CACHED");
            }
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
}
