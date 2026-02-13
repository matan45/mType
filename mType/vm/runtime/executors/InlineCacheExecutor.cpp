#include "InlineCacheExecutor.hpp"
#include "ObjectExecutor.hpp"
#include "FunctionExecutor.hpp"
#include "../utils/ErrorLocationHelper.hpp"
#include "../validation/AccessValidator.hpp"

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

        // Null check
        if (std::holds_alternative<std::nullptr_t>(objectValue))
        {
            const std::string& fieldName = context.program->getConstantPool().getString(instr.operands[0]);
            utils::ErrorLocationHelper::throwError<errors::NullPointerException>(context,
                "Cannot access field '" + fieldName + "' on null object");
        }

        // Must be an object
        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue))
        {
            // Fall back to generic path by pushing back and delegating
            context.stackManager->push(objectValue);
            objectExecutor->handleGetField(instr);
            return;
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);
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

        // Null check
        if (std::holds_alternative<std::nullptr_t>(objectValue))
        {
            utils::ErrorLocationHelper::throwError<errors::NullPointerException>(context,
                "Cannot set field '" + fieldName + "' on null object");
        }

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue))
        {
            // Fall back: push values back and delegate to generic handler
            context.stackManager->push(objectValue);
            context.stackManager->push(newValue);
            objectExecutor->handleSetField(instr);
            return;
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);
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

        if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue))
        {
            // Not a simple object — delegate to full handler (handles lambdas, etc.)
            objectExecutor->handleCallMethod(instr);
            return;
        }

        auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(objectValue);
        auto* classDef = instance->getClassDefinition().get();

        // IC fast path
        if (cache.state == ICState::MONOMORPHIC || cache.state == ICState::POLYMORPHIC)
        {
            const MethodICEntry* entry = cache.lookup(classDef);
            if (entry && entry->funcMetadata)
            {
                auto* funcMeta = static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(entry->funcMetadata);

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
                frame.functionName = entry->qualifiedName;
                frame.localBase = context.stackManager->size();
                frame.frameBase = context.stackManager->size();
                context.pushCallFrame(frame);

                // Enter function scope
                context.environment->enterScope();

                // Push 'this' as first local
                context.stackManager->push(objectValue);

                // Push arguments as locals
                for (const auto& arg : args)
                {
                    context.stackManager->push(arg);
                }

                // Jump to function start
                context.instructionPointer = entry->startOffset - 1; // -1 because loop increments
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
        objectExecutor->handleCallMethod(instr);

        // After the call, try to populate the cache for next time
        if (cache.state == ICState::UNINITIALIZED)
        {
            const std::string& methodName = context.program->getConstantPool().getString(instr.operands[0]);
            auto lookupResult = classDef->findInstanceMethodCached(methodName, argCount);
            if (lookupResult.method)
            {
                auto funcMeta = context.program->getFunction(lookupResult.qualifiedName);
                if (funcMeta)
                {
                    MethodICEntry entry;
                    entry.shape = classDef;
                    entry.funcMetadata = funcMeta;
                    entry.startOffset = funcMeta->startOffset;
                    entry.qualifiedName = lookupResult.qualifiedName;
                    cache.addEntry(entry);
                }
            }
        }
    }
}
