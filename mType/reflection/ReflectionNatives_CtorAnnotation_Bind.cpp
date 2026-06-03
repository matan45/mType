#include "ReflectionNatives.hpp"
#include <cstddef>
#include <cstdint>
#include "ReflectionHandle.hpp"
#include "../environment/Environment.hpp"
#include "../errors/RuntimeException.hpp"
#include "../value/InternedString.hpp"
#include "../value/NativeArray.hpp"
#include "../value/ObjectInstance.hpp"
#include "../value/ValueShim.hpp"
#include "../vm/runtime/VirtualMachine.hpp"

namespace reflection
{
    using namespace value;
    using namespace runtimeTypes::klass;

    Value ReflectionNatives::__reflect_newInstanceWithArgs(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 4, "__reflect_newInstanceWithArgs");

        if (!ctx.vm) {
            throw errors::RuntimeException("VM not initialized for reflection constructor invocation");
        }

        int64_t classHandle = extractInt(args[0], "__reflect_newInstanceWithArgs", "classHandle");
        int64_t ctorHandle = extractInt(args[1], "__reflect_newInstanceWithArgs", "ctorHandle");
        bool accessible = extractBool(args[3], "__reflect_newInstanceWithArgs", "accessible");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        auto ctorInfo = registry.getConstructor(ctorHandle);
        if (!ctorInfo.constructor)
        {
            throw errors::RuntimeException("Invalid constructor handle");
        }

        if (ctorInfo.classHandle != classHandle)
        {
            throw errors::RuntimeException("Constructor does not belong to class '" + classDef->getName() + "'");
        }

        if (!accessible && ctorInfo.constructor->getAccessModifier() != ast::AccessModifier::PUBLIC)
        {
            throw errors::RuntimeException("Cannot invoke non-public constructor without setting accessible to true");
        }

        std::vector<Value> argsVec;
        if (isNativeArray(args[2]))
        {
            auto argArray = asNativeArray(args[2]);
            for (size_t i = 0; i < argArray->size(); ++i)
            {
                argsVec.push_back(argArray->get(i));
            }
        }
        else if (!isVoid(args[2]) && !isNullType(args[2]))
        {
            throw errors::RuntimeException("Expected array for arguments parameter in __reflect_newInstanceWithArgs");
        }

        return ctx.vm->createObject(classDef->getName(), argsVec);
    }

    Value ReflectionNatives::__reflect_getClassAnnotations(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getClassAnnotations");
        int64_t classHandle = extractInt(args[0], "__reflect_getClassAnnotations", "classHandle");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto classDef = handleRegistry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        const auto& annotations = classDef->getAnnotations();
        std::vector<int> annotationHandles;

        for (const auto& annotation : annotations)
        {
            int64_t handle = handleRegistry.registerAnnotation(annotation, annotation->getName());
            annotationHandles.push_back(static_cast<int>(handle));
        }

        auto result = std::make_shared<NativeArray>(annotationHandles.size(), ValueType::INT);
        for (size_t i = 0; i < annotationHandles.size(); ++i)
        {
            result->set(static_cast<int>(i), annotationHandles[i]);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getClassAnnotation(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_getClassAnnotation");
        int64_t classHandle = extractInt(args[0], "__reflect_getClassAnnotation", "classHandle");
        std::string annotationName = extractString(args[1], "__reflect_getClassAnnotation", "annotationName");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto classDef = handleRegistry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        auto annotation = classDef->getAnnotation(annotationName);
        if (!annotation)
        {
            return static_cast<int>(0);
        }

        int64_t handle = handleRegistry.registerAnnotation(annotation, annotationName);
        return static_cast<int>(handle);
    }

    Value ReflectionNatives::__reflect_hasClassAnnotation(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_hasClassAnnotation");
        int64_t classHandle = extractInt(args[0], "__reflect_hasClassAnnotation", "classHandle");
        std::string annotationName = extractString(args[1], "__reflect_hasClassAnnotation", "annotationName");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto classDef = handleRegistry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        return classDef->hasAnnotation(annotationName);
    }

    Value ReflectionNatives::__reflect_getMethodAnnotations(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getMethodAnnotations");
        int64_t methodHandle = extractInt(args[0], "__reflect_getMethodAnnotations", "methodHandle");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto methodInfo = handleRegistry.getMethod(methodHandle);
        if (!methodInfo.method)
        {
            throw errors::RuntimeException("Invalid method handle");
        }

        const auto& annotations = methodInfo.method->getAnnotations();
        std::vector<int> annotationHandles;

        for (const auto& annotation : annotations)
        {
            int64_t handle = handleRegistry.registerAnnotation(annotation, annotation->getName());
            annotationHandles.push_back(static_cast<int>(handle));
        }

        auto result = std::make_shared<NativeArray>(annotationHandles.size(), ValueType::INT);
        for (size_t i = 0; i < annotationHandles.size(); ++i)
        {
            result->set(static_cast<int>(i), annotationHandles[i]);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getFieldAnnotations(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        // Fields don't have annotations in the current implementation.
        auto result = std::make_shared<NativeArray>(0, ValueType::INT);
        return result;
    }

    Value ReflectionNatives::__reflect_getAnnotationParam(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_getAnnotationParam");
        int64_t annotationHandle = extractInt(args[0], "__reflect_getAnnotationParam", "annotationHandle");
        std::string paramKey = extractString(args[1], "__reflect_getAnnotationParam", "paramKey");

        auto& registry = ReflectionHandleRegistry::instance();
        auto annotationInfo = registry.getAnnotation(annotationHandle);
        if (!annotationInfo.annotation)
        {
            throw errors::RuntimeException("Invalid annotation handle");
        }

        std::string paramValue = annotationInfo.annotation->getParameter(paramKey, "");
        if (paramValue.empty() && !annotationInfo.annotation->hasParameter(paramKey))
        {
            return std::monostate{};
        }
        return paramValue;
    }

    Value ReflectionNatives::__reflect_hasAnnotationParam(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_hasAnnotationParam");
        int64_t annotationHandle = extractInt(args[0], "__reflect_hasAnnotationParam", "annotationHandle");
        std::string paramKey = extractString(args[1], "__reflect_hasAnnotationParam", "paramKey");

        auto& registry = ReflectionHandleRegistry::instance();
        auto annotationInfo = registry.getAnnotation(annotationHandle);
        if (!annotationInfo.annotation)
        {
            throw errors::RuntimeException("Invalid annotation handle");
        }

        return annotationInfo.annotation->hasParameter(paramKey);
    }

    Value ReflectionNatives::__reflect_getAnnotationParams(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getAnnotationParams");
        int64_t annotationHandle = extractInt(args[0], "__reflect_getAnnotationParams", "annotationHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto annotationInfo = registry.getAnnotation(annotationHandle);
        if (!annotationInfo.annotation)
        {
            throw errors::RuntimeException("Invalid annotation handle");
        }

        const auto& params = annotationInfo.annotation->getParameters();
        std::vector<std::string> paramNames;
        paramNames.reserve(params.size());
        for (const auto& [key, value] : params)
        {
            paramNames.push_back(key);
        }

        auto result = std::make_shared<NativeArray>(paramNames.size(), ValueType::STRING);

        for (size_t i = 0; i < paramNames.size(); ++i)
        {
            result->set(static_cast<int>(i), paramNames[i]);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getMethodParameters(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getMethodParameters");
        int64_t methodHandle = extractInt(args[0], "__reflect_getMethodParameters", "methodHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto methodInfo = registry.getMethod(methodHandle);
        if (!methodInfo.method)
        {
            throw errors::RuntimeException("Invalid method handle");
        }

        const auto& params = methodInfo.method->getParameters();
        const bool skipThis = !methodInfo.method->isStatic() && !params.empty();
        const size_t startIndex = skipThis ? 1 : 0;
        const size_t userParamCount = params.size() - startIndex;

        auto result = std::make_shared<NativeArray>(userParamCount, ValueType::STRING);
        for (size_t i = startIndex; i < params.size(); ++i)
        {
            result->set(static_cast<int>(i - startIndex), params[i].first);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getConstructorParameters(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getConstructorParameters");
        int64_t ctorHandle = extractInt(args[0], "__reflect_getConstructorParameters", "ctorHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto ctorInfo = registry.getConstructor(ctorHandle);
        if (!ctorInfo.constructor)
        {
            throw errors::RuntimeException("Invalid constructor handle");
        }

        const auto& params = ctorInfo.constructor->getParametersWithTypes();

        auto result = std::make_shared<NativeArray>(params.size(), ValueType::STRING);
        for (size_t i = 0; i < params.size(); ++i)
        {
            result->set(static_cast<int>(i), params[i].first);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getParameterType(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getParameterType");
        int64_t typeHandle = extractInt(args[0], "__reflect_getParameterType", "typeHandle");

        // typeHandle is actually a class handle in this context.
        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(typeHandle);
        if (classDef)
        {
            return classDef->getName();
        }

        return "unknown";
    }

    // Annotation lookup-by-name natives. Each pair (get/has) walks the
    // annotation list on the target handle's definition (method/field/ctor)
    // and either registers a handle for the matched annotation (get*) or
    // returns 0 on miss so the mType wrapper can materialize null.

    Value ReflectionNatives::__reflect_getMethodAnnotation(void* /*userData*/, environment::NativeContext& /*ctx*/, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_getMethodAnnotation");
        int64_t methodHandle = extractInt(args[0], "__reflect_getMethodAnnotation", "methodHandle");
        std::string annotationName = extractString(args[1], "__reflect_getMethodAnnotation", "annotationName");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto methodInfo = handleRegistry.getMethod(methodHandle);
        if (!methodInfo.method)
        {
            throw errors::RuntimeException("Invalid method handle");
        }

        for (const auto& annotation : methodInfo.method->getAnnotations())
        {
            if (annotation->getName() == annotationName)
            {
                int64_t handle = handleRegistry.registerAnnotation(annotation, annotation->getName());
                return static_cast<int>(handle);
            }
        }
        return static_cast<int>(0);
    }

    Value ReflectionNatives::__reflect_hasMethodAnnotation(void* /*userData*/, environment::NativeContext& /*ctx*/, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_hasMethodAnnotation");
        int64_t methodHandle = extractInt(args[0], "__reflect_hasMethodAnnotation", "methodHandle");
        std::string annotationName = extractString(args[1], "__reflect_hasMethodAnnotation", "annotationName");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto methodInfo = handleRegistry.getMethod(methodHandle);
        if (!methodInfo.method)
        {
            throw errors::RuntimeException("Invalid method handle");
        }

        for (const auto& annotation : methodInfo.method->getAnnotations())
        {
            if (annotation->getName() == annotationName) return true;
        }
        return false;
    }

    Value ReflectionNatives::__reflect_getFieldAnnotation(void* /*userData*/, environment::NativeContext& /*ctx*/, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_getFieldAnnotation");
        int64_t fieldHandle = extractInt(args[0], "__reflect_getFieldAnnotation", "fieldHandle");
        std::string annotationName = extractString(args[1], "__reflect_getFieldAnnotation", "annotationName");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto fieldInfo = handleRegistry.getField(fieldHandle);
        if (!fieldInfo.field)
        {
            throw errors::RuntimeException("Invalid field handle");
        }

        for (const auto& annotation : fieldInfo.field->getAnnotations())
        {
            if (annotation->getName() == annotationName)
            {
                int64_t handle = handleRegistry.registerAnnotation(annotation, annotation->getName());
                return static_cast<int>(handle);
            }
        }
        return static_cast<int>(0);
    }

    Value ReflectionNatives::__reflect_hasFieldAnnotation(void* /*userData*/, environment::NativeContext& /*ctx*/, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_hasFieldAnnotation");
        int64_t fieldHandle = extractInt(args[0], "__reflect_hasFieldAnnotation", "fieldHandle");
        std::string annotationName = extractString(args[1], "__reflect_hasFieldAnnotation", "annotationName");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto fieldInfo = handleRegistry.getField(fieldHandle);
        if (!fieldInfo.field)
        {
            throw errors::RuntimeException("Invalid field handle");
        }

        for (const auto& annotation : fieldInfo.field->getAnnotations())
        {
            if (annotation->getName() == annotationName) return true;
        }
        return false;
    }

    Value ReflectionNatives::__reflect_getConstructorAnnotations(void* /*userData*/, environment::NativeContext& /*ctx*/, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getConstructorAnnotations");
        int64_t ctorHandle = extractInt(args[0], "__reflect_getConstructorAnnotations", "constructorHandle");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto ctorInfo = handleRegistry.getConstructor(ctorHandle);
        if (!ctorInfo.constructor)
        {
            throw errors::RuntimeException("Invalid constructor handle");
        }

        const auto& annotations = ctorInfo.constructor->getAnnotations();
        auto result = std::make_shared<NativeArray>(annotations.size(), ValueType::INT);
        for (size_t i = 0; i < annotations.size(); ++i)
        {
            int64_t handle = handleRegistry.registerAnnotation(annotations[i], annotations[i]->getName());
            result->set(static_cast<int>(i), static_cast<int>(handle));
        }
        return result;
    }

    Value ReflectionNatives::__reflect_getConstructorAnnotation(void* /*userData*/, environment::NativeContext& /*ctx*/, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_getConstructorAnnotation");
        int64_t ctorHandle = extractInt(args[0], "__reflect_getConstructorAnnotation", "constructorHandle");
        std::string annotationName = extractString(args[1], "__reflect_getConstructorAnnotation", "annotationName");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto ctorInfo = handleRegistry.getConstructor(ctorHandle);
        if (!ctorInfo.constructor)
        {
            throw errors::RuntimeException("Invalid constructor handle");
        }

        for (const auto& annotation : ctorInfo.constructor->getAnnotations())
        {
            if (annotation->getName() == annotationName)
            {
                int64_t handle = handleRegistry.registerAnnotation(annotation, annotation->getName());
                return static_cast<int>(handle);
            }
        }
        return static_cast<int>(0);
    }

    Value ReflectionNatives::__reflect_hasConstructorAnnotation(void* /*userData*/, environment::NativeContext& /*ctx*/, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_hasConstructorAnnotation");
        int64_t ctorHandle = extractInt(args[0], "__reflect_hasConstructorAnnotation", "constructorHandle");
        std::string annotationName = extractString(args[1], "__reflect_hasConstructorAnnotation", "annotationName");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto ctorInfo = handleRegistry.getConstructor(ctorHandle);
        if (!ctorInfo.constructor)
        {
            throw errors::RuntimeException("Invalid constructor handle");
        }

        for (const auto& annotation : ctorInfo.constructor->getAnnotations())
        {
            if (annotation->getName() == annotationName) return true;
        }
        return false;
    }

    Value ReflectionNatives::__reflect_getAnnotationMeta(void* /*userData*/, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_getAnnotationMeta");
        int64_t annotationHandle = extractInt(args[0], "__reflect_getAnnotationMeta", "annotationHandle");
        std::string metaName = extractString(args[1], "__reflect_getAnnotationMeta", "metaName");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto annotationInfo = handleRegistry.getAnnotation(annotationHandle);
        if (!annotationInfo.annotation)
        {
            throw errors::RuntimeException("Invalid annotation handle");
        }

        if (!ctx.env)
        {
            throw errors::RuntimeException("Reflection environment not initialized");
        }

        auto annotationRegistry = ctx.env->getAnnotationRegistry();
        if (!annotationRegistry)
        {
            return static_cast<int>(0);
        }

        const std::string annotationName = annotationInfo.annotationName.empty()
            ? annotationInfo.annotation->getName()
            : annotationInfo.annotationName;
        auto annotationDefinition = annotationRegistry->findAnnotation(annotationName);
        if (!annotationDefinition)
        {
            return static_cast<int>(0);
        }

        auto metaAnnotation = annotationDefinition->getMetaAnnotation(metaName);
        if (!metaAnnotation)
        {
            return static_cast<int>(0);
        }

        int64_t metaHandle = handleRegistry.registerAnnotation(metaAnnotation, metaAnnotation->getName());
        return static_cast<int>(metaHandle);
    }
}
