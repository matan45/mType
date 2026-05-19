#include "ReflectionNatives.hpp"
#include <cstddef>
#include <cstdint>
#include "ReflectionHandle.hpp"
#include "../errors/RuntimeException.hpp"
#include "../value/InternedString.hpp"
#include "../value/NativeArray.hpp"
#include "../value/ObjectInstance.hpp"
#include "../value/ValueShim.hpp"

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

    Value ReflectionNatives::__reflect_getConstructorDeclaringClass(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getConstructorDeclaringClass");
        int64_t classHandle = extractInt(args[0], "__reflect_getConstructorDeclaringClass", "classHandle");
        return classHandle;
    }
}
