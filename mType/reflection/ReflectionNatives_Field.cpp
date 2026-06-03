#include "ReflectionNatives.hpp"
#include <cstddef>
#include <cstdint>
#include "ReflectionHandle.hpp"
#include "../errors/RuntimeException.hpp"
#include "../value/ObjectInstance.hpp"
#include "../value/NativeArray.hpp"
#include "../value/InternedString.hpp"

namespace reflection
{
    using namespace value;
    using namespace runtimeTypes::klass;


    Value ReflectionNatives::__reflect_getField(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 3, "__reflect_getField");
        int64_t classHandle = extractInt(args[0], "__reflect_getField", "classHandle");
        const std::string& fieldName = extractString(args[1], "__reflect_getField", "fieldName");
        bool declaredOnly = extractBool(args[2], "__reflect_getField", "declaredOnly");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto classDef = handleRegistry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        int64_t cachedHandle = handleRegistry.findCachedFieldLookup(
            classHandle, fieldName, declaredOnly);
        if (cachedHandle != -1)
        {
            return static_cast<int>(cachedHandle);
        }

        std::shared_ptr<FieldDefinition> fieldDef = nullptr;

        // Check instance fields
        const auto& instanceFields = classDef->getInstanceFields();
        auto it = instanceFields.find(fieldName);
        if (it != instanceFields.end())
        {
            fieldDef = it->second;
        }

        // Check static fields
        if (!fieldDef)
        {
            const auto& staticFields = classDef->getStaticFields();
            auto sit = staticFields.find(fieldName);
            if (sit != staticFields.end())
            {
                fieldDef = sit->second;
            }
        }

        // Search hierarchy if not declared only
        if (!fieldDef && !declaredOnly)
        {
            fieldDef = classDef->getFieldInHierarchy(fieldName);
        }

        if (!fieldDef)
        {
            throw errors::RuntimeException("Field not found: " + fieldName);
        }

        // If not declaredOnly and field is not public, throw exception
        if (!declaredOnly && fieldDef->getAccessModifier() != ast::AccessModifier::PUBLIC)
        {
            throw errors::RuntimeException("Field " + fieldName + " is not public. Use getDeclaredField() instead.");
        }

        int64_t fieldHandle = handleRegistry.registerField(fieldDef, classHandle, fieldName);
        handleRegistry.cacheFieldLookup(classHandle, fieldName, declaredOnly, fieldHandle);
        return static_cast<int>(fieldHandle);
    }

    Value ReflectionNatives::__reflect_getFields(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_getFields");
        int64_t classHandle = extractInt(args[0], "__reflect_getFields", "classHandle");
        bool declaredOnly = extractBool(args[1], "__reflect_getFields", "declaredOnly");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto classDef = handleRegistry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        std::vector<int> fieldHandles;

        // Collect instance fields in declaration order.
        const auto& instanceFields = classDef->getInstanceFields();
        for (const auto& name : classDef->getInstanceFieldOrder())
        {
            auto it = instanceFields.find(name);
            if (it == instanceFields.end()) continue;
            const auto& fieldDef = it->second;
            if (declaredOnly || fieldDef->getAccessModifier() == ast::AccessModifier::PUBLIC)
            {
                int64_t handle = handleRegistry.registerField(fieldDef, classHandle, name);
                fieldHandles.push_back(static_cast<int>(handle));
            }
        }

        // Collect static fields in declaration order.
        const auto& staticFields = classDef->getStaticFields();
        for (const auto& name : classDef->getStaticFieldOrder())
        {
            auto it = staticFields.find(name);
            if (it == staticFields.end()) continue;
            const auto& fieldDef = it->second;
            if (declaredOnly || fieldDef->getAccessModifier() == ast::AccessModifier::PUBLIC)
            {
                int64_t handle = handleRegistry.registerField(fieldDef, classHandle, name);
                fieldHandles.push_back(static_cast<int>(handle));
            }
        }

        // Create result array
        auto result = std::make_shared<NativeArray>(fieldHandles.size(), ValueType::INT);
        for (size_t i = 0; i < fieldHandles.size(); ++i)
        {
            result->set(static_cast<int>(i), fieldHandles[i]);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getFieldType(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getFieldType");
        int64_t fieldHandle = extractInt(args[0], "__reflect_getFieldType", "fieldHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto fieldInfo = registry.getField(fieldHandle);
        if (!fieldInfo.field)
        {
            throw errors::RuntimeException("Invalid field handle");
        }

        if (fieldInfo.field->hasUnifiedType())
        {
            auto unifiedType = fieldInfo.field->getUnifiedType();
            if (unifiedType)
            {
                std::string typeName = unifiedType->toString();
                if (!typeName.empty())
                {
                    return typeName;
                }
            }
        }

        return valueTypeToTypeName(fieldInfo.field->getType());
    }

    Value ReflectionNatives::__reflect_getFieldDeclaringClass(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getFieldDeclaringClass");
        int64_t classHandle = extractInt(args[0], "__reflect_getFieldDeclaringClass", "classHandle");

        // The classHandle is already the declaring class handle
        return classHandle;
    }

    Value ReflectionNatives::__reflect_getFieldModifiers(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getFieldModifiers");
        int64_t fieldHandle = extractInt(args[0], "__reflect_getFieldModifiers", "fieldHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto fieldInfo = registry.getField(fieldHandle);
        if (!fieldInfo.field)
        {
            throw errors::RuntimeException("Invalid field handle");
        }

        return fieldInfo.field->getModifierFlags();
    }

    Value ReflectionNatives::__reflect_getFieldValue(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 3, "__reflect_getFieldValue");
        auto instance = extractObject(args[0], "__reflect_getFieldValue", "instance");
        int64_t fieldHandle = extractInt(args[1], "__reflect_getFieldValue", "fieldHandle");
        bool accessible = extractBool(args[2], "__reflect_getFieldValue", "accessible");

        auto& registry = ReflectionHandleRegistry::instance();
        auto fieldInfo = registry.getField(fieldHandle);
        if (!fieldInfo.field)
        {
            throw errors::RuntimeException("Invalid field handle");
        }

        // Check access control - without caller context, we can't verify protected access properly,
        // so we treat both private and protected as requiring setAccessible(true)
        if (!accessible)
        {
            auto accessMod = fieldInfo.field->getAccessModifier();
            if (accessMod == ast::AccessModifier::PRIVATE)
            {
                throw errors::RuntimeException("Cannot access private field. Call setAccessible(true) first.");
            }
            if (accessMod == ast::AccessModifier::PROTECTED)
            {
                throw errors::RuntimeException("Cannot access protected field. Call setAccessible(true) first.");
            }
        }

        return instance->getFieldValue(fieldInfo.fieldName);
    }

    Value ReflectionNatives::__reflect_setFieldValue(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 4, "__reflect_setFieldValue");
        auto instance = extractObject(args[0], "__reflect_setFieldValue", "instance");
        int64_t fieldHandle = extractInt(args[1], "__reflect_setFieldValue", "fieldHandle");
        const Value& newValue = args[2];
        bool accessible = extractBool(args[3], "__reflect_setFieldValue", "accessible");

        auto& registry = ReflectionHandleRegistry::instance();
        auto fieldInfo = registry.getField(fieldHandle);
        if (!fieldInfo.field)
        {
            throw errors::RuntimeException("Invalid field handle");
        }

        // Check access control - without caller context, we can't verify protected access properly,
        // so we treat both private and protected as requiring setAccessible(true)
        if (!accessible)
        {
            auto accessMod = fieldInfo.field->getAccessModifier();
            if (accessMod == ast::AccessModifier::PRIVATE)
            {
                throw errors::RuntimeException("Cannot access private field. Call setAccessible(true) first.");
            }
            if (accessMod == ast::AccessModifier::PROTECTED)
            {
                throw errors::RuntimeException("Cannot access protected field. Call setAccessible(true) first.");
            }
        }

        // Check final field - accessible bypasses this too
        if (fieldInfo.field->isFinal() && fieldInfo.field->isInitialized() && !accessible)
        {
            throw errors::RuntimeException("Cannot modify final field. Call setAccessible(true) first.");
        }

        instance->setField(fieldInfo.fieldName, newValue);
        return std::monostate{};
    }

    Value ReflectionNatives::__reflect_getStaticFieldValue(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_getStaticFieldValue");
        int64_t classHandle = extractInt(args[0], "__reflect_getStaticFieldValue", "classHandle");
        int64_t fieldHandle = extractInt(args[1], "__reflect_getStaticFieldValue", "fieldHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto fieldInfo = registry.getField(fieldHandle);
        if (!fieldInfo.field)
        {
            throw errors::RuntimeException("Invalid field handle");
        }

        if (!fieldInfo.field->isStatic())
        {
            throw errors::RuntimeException("Field is not static");
        }

        return fieldInfo.field->getValue();
    }

    Value ReflectionNatives::__reflect_setStaticFieldValue(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 3, "__reflect_setStaticFieldValue");
        int64_t classHandle = extractInt(args[0], "__reflect_setStaticFieldValue", "classHandle");
        int64_t fieldHandle = extractInt(args[1], "__reflect_setStaticFieldValue", "fieldHandle");
        const Value& newValue = args[2];

        auto& registry = ReflectionHandleRegistry::instance();
        auto fieldInfo = registry.getField(fieldHandle);
        if (!fieldInfo.field)
        {
            throw errors::RuntimeException("Invalid field handle");
        }

        if (!fieldInfo.field->isStatic())
        {
            throw errors::RuntimeException("Field is not static");
        }

        fieldInfo.field->setValue(newValue);
        return std::monostate{};
    }

    Value ReflectionNatives::__reflect_getFieldName(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getFieldName");
        int64_t fieldHandle = extractInt(args[0], "__reflect_getFieldName", "fieldHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto fieldInfo = registry.getField(fieldHandle);
        if (!fieldInfo.field)
        {
            throw errors::RuntimeException("Invalid field handle");
        }

        return fieldInfo.fieldName;
    }


} // namespace reflection



