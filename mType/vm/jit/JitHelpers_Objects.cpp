#include "JitHelpers.hpp"
#include "ic/InlineCacheTable.hpp"
#include "../../errors/AccessViolationException.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../errors/NullPointerException.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
#include "../../environment/Environment.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../value/ValueObject.hpp"
#include <vector>
#include <memory>

namespace vm::jit
{
    void jit_call_method(JitContext* ctx, uint32_t methodNameIndex, size_t argCount)
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
        const std::string& className = ctx->program->getConstantPool().getString(classIndex);
        std::vector<value::Value> args(ctx->callArgs, ctx->callArgs + argCount);

        if (ctx->vm)
        {
            *dest = ctx->vm->createObject(className, args);
            return;
        }

        throw errors::RuntimeException("JIT: cannot create object '" + className + "'");
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

    static bool getFieldFromValueObject(value::Value* dest,
                                        const value::Value* object,
                                        JitContext* ctx,
                                        uint32_t fieldNameIndex)
    {
        if (!std::holds_alternative<std::shared_ptr<value::ValueObject>>(*object))
            return false;

        auto valueObj = std::get<std::shared_ptr<value::ValueObject>>(*object);
        auto classDef = valueObj->getClassDefinition();
        const std::string& fieldName =
            ctx->program->getConstantPool().getString(fieldNameIndex);
        size_t fieldIndex = classDef->getFieldIndex(fieldName);
        if (fieldIndex != SIZE_MAX && fieldIndex < valueObj->getFieldCount())
        {
            *dest = valueObj->getFieldByIndex(fieldIndex);
            return true;
        }
        *dest = valueObj->getFieldValue(fieldName);
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
            if (std::holds_alternative<std::nullptr_t>(*object) ||
                std::holds_alternative<std::monostate>(*object))
            {
                throw errors::NullPointerException("Cannot access field on null");
            }
        }

        if (getFieldFromValueObject(dest, object, ctx, fieldNameIndex))
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
            if (std::holds_alternative<std::nullptr_t>(*object) ||
                std::holds_alternative<std::monostate>(*object))
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
