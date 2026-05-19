#include "ReflectionNatives.hpp"
#include <cstddef>
#include <cstdint>
#include "ReflectionHandle.hpp"
#include "../errors/RuntimeException.hpp"
#include "../types/UnifiedType.hpp"
#include "../value/InternedString.hpp"
#include "../value/NativeArray.hpp"
#include "../value/ValueShim.hpp"

namespace reflection
{
    using namespace value;
    using namespace runtimeTypes::klass;

    Value ReflectionNatives::__reflect_getInterfaces(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getInterfaces");
        int64_t classHandle = extractInt(args[0], "__reflect_getInterfaces", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        const auto& interfaces = classDef->getImplementedInterfaces();
        auto result = std::make_shared<NativeArray>(interfaces.size(), ValueType::STRING);
        for (size_t i = 0; i < interfaces.size(); ++i)
        {
            result->set(static_cast<int>(i), interfaces[i]);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_isGenericClass(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_isGenericClass");
        int64_t classHandle = extractInt(args[0], "__reflect_isGenericClass", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        return classDef->isGeneric();
    }

    Value ReflectionNatives::__reflect_getTypeParameters(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getTypeParameters");
        int64_t classHandle = extractInt(args[0], "__reflect_getTypeParameters", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        const auto& params = classDef->getGenericParameters();
        auto result = std::make_shared<NativeArray>(params.size(), ValueType::STRING);

        for (size_t i = 0; i < params.size(); ++i)
        {
            result->set(i, params[i].name);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getTypeArguments(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getTypeArguments");
        int64_t classHandle = extractInt(args[0], "__reflect_getTypeArguments", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        // Open template handles and non-generic classes return an empty array.
        auto reified = registry.getReifiedType(classHandle);
        if (!reified || reified->getTypeArguments().empty())
        {
            return std::make_shared<NativeArray>(0, ValueType::INT);
        }

        const auto& typeArgs = reified->getTypeArguments();
        auto result = std::make_shared<NativeArray>(typeArgs.size(), ValueType::INT);

        for (size_t i = 0; i < typeArgs.size(); ++i)
        {
            const auto& arg = typeArgs[i];
            if (!arg)
            {
                throw errors::RuntimeException("Invalid null type argument in reified type");
            }

            auto argBaseDef = ctx.env->findClass(arg->getName());
            if (!argBaseDef)
            {
                throw errors::RuntimeException("Class not found: " + arg->getName());
            }

            int64_t argHandle;
            if (arg->isParameterized())
            {
                // Nested closed form (e.g. List<Int> inside Map<String,List<Int>>):
                // mint a closed handle so the caller can recursively introspect.
                argHandle = registry.getOrCreateClosedClassHandle(argBaseDef, arg);
            }
            else
            {
                argHandle = registry.getOrCreateClassHandle(argBaseDef);
            }
            result->set(static_cast<int>(i), argHandle);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getClassModifiers(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getClassModifiers");
        int64_t classHandle = extractInt(args[0], "__reflect_getClassModifiers", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        return classDef->getModifierFlags();
    }

    Value ReflectionNatives::__reflect_getName(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getName");
        int64_t classHandle = extractInt(args[0], "__reflect_getName", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        // Closed-form handles render the parameterized canonical string
        // (e.g. "Box<Int>"); open-form handles return the raw class name.
        auto reified = registry.getReifiedType(classHandle);
        if (reified)
        {
            return reified->toCanonicalString();
        }
        return classDef->getName();
    }

    Value ReflectionNatives::__reflect_getRawName(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getRawName");
        int64_t classHandle = extractInt(args[0], "__reflect_getRawName", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        // Raw name ALWAYS comes from the ClassDefinition, never rendered with
        // type arguments — matches ValueObject::getClassName().
        return classDef->getName();
    }
}
