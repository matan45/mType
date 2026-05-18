#include "ReflectionNatives.hpp"
#include <cstddef>
#include <cstdint>
#include "ReflectionHandle.hpp"
#include "../errors/RuntimeException.hpp"
#include "../value/ObjectInstance.hpp"
#include "../value/NativeArray.hpp"
#include "../value/InternedString.hpp"
#include "../value/ValueShim.hpp"
#include "../vm/runtime/VirtualMachine.hpp"

namespace reflection
{
    using namespace value;
    using namespace runtimeTypes::klass;

    namespace
    {
        Value getConstructorByArityImpl(
            int64_t classHandle,
            size_t paramCount,
            bool declaredOnly)
        {
            auto& handleRegistry = ReflectionHandleRegistry::instance();
            auto classDef = handleRegistry.getClass(classHandle);
            if (!classDef)
            {
                throw errors::RuntimeException("Invalid class handle");
            }

            int64_t cachedHandle = handleRegistry.findCachedConstructorLookup(
                classHandle, paramCount, declaredOnly);
            if (cachedHandle != -1)
            {
                return static_cast<int>(cachedHandle);
            }

            auto* ctorDef = classDef->findConstructor(paramCount);
            if (!ctorDef)
            {
                throw errors::RuntimeException("Constructor not found with " + std::to_string(paramCount) + " parameters");
            }

            if (!declaredOnly && ctorDef->getAccessModifier() != ast::AccessModifier::PUBLIC)
            {
                throw errors::RuntimeException("Constructor is not public. Use getDeclaredConstructor() instead.");
            }

            const auto& constructors = classDef->getConstructors();
            for (size_t i = 0; i < constructors.size(); ++i)
            {
                if (constructors[i].get() == ctorDef)
                {
                    int64_t ctorHandle = handleRegistry.registerConstructor(
                        constructors[i], classHandle, static_cast<int>(i));
                    handleRegistry.cacheConstructorLookup(
                        classHandle, paramCount, declaredOnly, ctorHandle);
                    return static_cast<int>(ctorHandle);
                }
            }

            throw errors::RuntimeException("Constructor registration failed");
        }
    }

    Value ReflectionNatives::__reflect_getConstructor(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 3, "__reflect_getConstructor");
        int64_t classHandle = extractInt(args[0], "__reflect_getConstructor", "classHandle");
        // args[1] is paramTypes array
        bool declaredOnly = extractBool(args[2], "__reflect_getConstructor", "declaredOnly");

        size_t paramCount = 0;
        if (isNativeArray(args[1]))
        {
            auto paramArray = asNativeArray(args[1]);
            paramCount = paramArray->size();
        }

        return getConstructorByArityImpl(classHandle, paramCount, declaredOnly);
    }

    Value ReflectionNatives::__reflect_getConstructorByArity(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 3, "__reflect_getConstructorByArity");
        int64_t classHandle = extractInt(args[0], "__reflect_getConstructorByArity", "classHandle");
        int64_t rawParamCount = extractInt(args[1], "__reflect_getConstructorByArity", "paramCount");
        bool declaredOnly = extractBool(args[2], "__reflect_getConstructorByArity", "declaredOnly");

        if (rawParamCount < 0)
        {
            throw errors::RuntimeException("__reflect_getConstructorByArity requires paramCount to be non-negative");
        }

        return getConstructorByArityImpl(classHandle, static_cast<size_t>(rawParamCount), declaredOnly);
    }

    Value ReflectionNatives::__reflect_getConstructors(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_getConstructors");
        int64_t classHandle = extractInt(args[0], "__reflect_getConstructors", "classHandle");
        bool declaredOnly = extractBool(args[1], "__reflect_getConstructors", "declaredOnly");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto classDef = handleRegistry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        std::vector<int> ctorHandles;
        const auto& constructors = classDef->getConstructors();

        for (size_t i = 0; i < constructors.size(); ++i)
        {
            if (declaredOnly || constructors[i]->getAccessModifier() == ast::AccessModifier::PUBLIC)
            {
                int64_t handle = handleRegistry.registerConstructor(constructors[i], classHandle, static_cast<int>(i));
                ctorHandles.push_back(static_cast<int>(handle));
            }
        }

        auto result = std::make_shared<NativeArray>(ctorHandles.size(), ValueType::INT);
        for (size_t i = 0; i < ctorHandles.size(); ++i)
        {
            result->set(static_cast<int>(i), ctorHandles[i]);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getConstructorParamTypes(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getConstructorParamTypes");
        int64_t ctorHandle = extractInt(args[0], "__reflect_getConstructorParamTypes", "ctorHandle");

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
            result->set(static_cast<int>(i), valueTypeToTypeName(params[i].second.basicType));
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getConstructorParamCount(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getConstructorParamCount");
        int64_t ctorHandle = extractInt(args[0], "__reflect_getConstructorParamCount", "ctorHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto ctorInfo = registry.getConstructor(ctorHandle);
        if (!ctorInfo.constructor)
        {
            throw errors::RuntimeException("Invalid constructor handle");
        }

        return static_cast<int>(ctorInfo.constructor->getParameterCount());
    }

    Value ReflectionNatives::__reflect_getConstructorModifiers(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getConstructorModifiers");
        int64_t ctorHandle = extractInt(args[0], "__reflect_getConstructorModifiers", "ctorHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto ctorInfo = registry.getConstructor(ctorHandle);
        if (!ctorInfo.constructor)
        {
            throw errors::RuntimeException("Invalid constructor handle");
        }

        return ctorInfo.constructor->getModifierFlags();
    }

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

    Value ReflectionNatives::__reflect_getConstructorDeclaringClass(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getConstructorDeclaringClass");
        int64_t classHandle = extractInt(args[0], "__reflect_getConstructorDeclaringClass", "classHandle");
        return classHandle;
    }

    // ========== Annotation Reflection Implementations ==========

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
            return std::monostate{}; // null
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
        // Fields don't have annotations in current implementation
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

        // Get parameter value from annotation node
        // Returns empty string if not found
        std::string paramValue = annotationInfo.annotation->getParameter(paramKey, "");
        if (paramValue.empty() && !annotationInfo.annotation->hasParameter(paramKey))
        {
            return std::monostate{}; // null if not found
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

        // Get parameters map and extract keys
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

    // ========== Parameter Reflection Implementations ==========

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

        // Return array of parameter names
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

        // typeHandle is actually a class handle in this context
        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(typeHandle);
        if (classDef)
        {
            return classDef->getName();
        }

        return "unknown";
    }

    // -------------------------------------------------------------------
    // Annotation lookup-by-name natives.
    //
    // Each pair (get/has) walks the annotation list on the target handle's
    // definition (method/field/constructor) and either registers a handle
    // for the matched annotation (`get*`, returns -1 on miss) or returns
    // a bool (`has*`).
    // -------------------------------------------------------------------

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
        return static_cast<int>(-1);
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
        return static_cast<int>(-1);
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
        return static_cast<int>(-1);
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

    Value ReflectionNatives::__reflect_getAnnotationMeta(void* /*userData*/, environment::NativeContext& /*ctx*/, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_getAnnotationMeta");
        int64_t annotationHandle = extractInt(args[0], "__reflect_getAnnotationMeta", "annotationHandle");
        // metaName accepted but unused: AnnotationNode does not currently track
        // meta-annotations, so this native always returns -1 (not found).
        (void)extractString(args[1], "__reflect_getAnnotationMeta", "metaName");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto annotationInfo = handleRegistry.getAnnotation(annotationHandle);
        if (!annotationInfo.annotation)
        {
            throw errors::RuntimeException("Invalid annotation handle");
        }

        // No meta-annotation infrastructure on AnnotationNode yet.
        return static_cast<int>(-1);
    }


} // namespace reflection







