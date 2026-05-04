#include "ReflectionNatives.hpp"
#include "ReflectionHandle.hpp"
#include "../errors/RuntimeException.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../value/NativeArray.hpp"
#include "../value/InternedString.hpp"
#include "../value/ValueShim.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../vm/MethodSignature.hpp"
#include <algorithm>
#include <unordered_set>

namespace reflection
{
    using namespace value;
    using namespace runtimeTypes::klass;


    Value ReflectionNatives::__reflect_getMethod(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 4, "__reflect_getMethod");
        int64_t classHandle = extractInt(args[0], "__reflect_getMethod", "classHandle");
        std::string methodName = extractString(args[1], "__reflect_getMethod", "methodName");
        // args[2] is paramTypes array - for simplicity, we'll use argCount-based lookup
        bool declaredOnly = extractBool(args[3], "__reflect_getMethod", "declaredOnly");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto classDef = handleRegistry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        // Get param count from array
        size_t paramCount = 0;
        if (isNativeArray(args[2]))
        {
            auto paramArray = asNativeArray(args[2]);
            paramCount = paramArray->size();
        }

        int64_t cachedHandle = handleRegistry.findCachedMethodLookup(
            classHandle, methodName, paramCount, declaredOnly);
        if (cachedHandle != -1)
        {
            return static_cast<int>(cachedHandle);
        }

        std::shared_ptr<MethodDefinition> methodDef = nullptr;

        if (declaredOnly)
        {
            methodDef = classDef->findInstanceMethod(methodName, paramCount);
            if (!methodDef)
            {
                methodDef = classDef->findStaticMethod(methodName, paramCount);
            }
        }
        else
        {
            methodDef = classDef->findInstanceMethodInHierarchy(methodName, paramCount);
            if (!methodDef)
            {
                methodDef = classDef->findStaticMethodInHierarchy(methodName, paramCount);
            }
        }

        if (!methodDef)
        {
            throw errors::RuntimeException("Method not found: " + methodName);
        }

        if (!declaredOnly && methodDef->getAccessModifier() != ast::AccessModifier::PUBLIC)
        {
            throw errors::RuntimeException("Method " + methodName + " is not public. Use getDeclaredMethod() instead.");
        }

        int64_t methodHandle = handleRegistry.registerMethod(methodDef, classHandle, methodName);
        handleRegistry.cacheMethodLookup(classHandle, methodName, paramCount, declaredOnly, methodHandle);
        return static_cast<int>(methodHandle);
    }

    Value ReflectionNatives::__reflect_getMethods(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_getMethods");
        int64_t classHandle = extractInt(args[0], "__reflect_getMethods", "classHandle");
        bool declaredOnly = extractBool(args[1], "__reflect_getMethods", "declaredOnly");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto classDef = handleRegistry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        std::vector<int> methodHandles;

        if (declaredOnly)
        {
            // Declared-only: this class's methods only, all access levels.
            // MYT-274: filter compiler-synthesized methods (auto hashCode/equals).
            for (const auto& [name, overloads] : classDef->getInstanceMethods())
            {
                for (const auto& methodDef : overloads)
                {
                    if (methodDef->isSynthetic()) continue;
                    int64_t handle = handleRegistry.registerMethod(methodDef, classHandle, name);
                    methodHandles.push_back(static_cast<int>(handle));
                }
            }
            for (const auto& [name, overloads] : classDef->getStaticMethods())
            {
                for (const auto& methodDef : overloads)
                {
                    if (methodDef->isSynthetic()) continue;
                    int64_t handle = handleRegistry.registerMethod(methodDef, classHandle, name);
                    methodHandles.push_back(static_cast<int>(handle));
                }
            }
        }
        else
        {
            // MYT-213: walk the parent chain, dedup overrides by (name + signature)
            // so child overrides hide parent versions. Public-only filter.
            // MYT-274: also filter compiler-synthesized methods.
            std::unordered_set<std::string> seenSigs;
            constexpr int MAX_DEPTH = 20;

            auto emitMethod = [&](const std::shared_ptr<MethodDefinition>& methodDef,
                                  const std::string& name,
                                  bool isStatic)
            {
                if (methodDef->getAccessModifier() != ast::AccessModifier::PUBLIC) return;
                if (methodDef->isSynthetic()) return;
                auto sig = vm::MethodSignature::fromMethodDefinition(methodDef.get());
                std::string key = (isStatic ? "S:" : "I:") + sig.toMangledName(name, isStatic);
                if (!seenSigs.insert(key).second) return;
                int64_t handle = handleRegistry.registerMethod(methodDef, classHandle, name);
                methodHandles.push_back(static_cast<int>(handle));
            };

            auto current = classDef;
            int depth = 0;
            while (current && depth < MAX_DEPTH)
            {
                for (const auto& [name, overloads] : current->getInstanceMethods())
                {
                    for (const auto& methodDef : overloads)
                    {
                        emitMethod(methodDef, name, /*isStatic*/ false);
                    }
                }
                for (const auto& [name, overloads] : current->getStaticMethods())
                {
                    for (const auto& methodDef : overloads)
                    {
                        emitMethod(methodDef, name, /*isStatic*/ true);
                    }
                }
                current = current->getParentClass();
                ++depth;
            }
        }

        auto result = std::make_shared<NativeArray>(methodHandles.size(), ValueType::INT);
        for (size_t i = 0; i < methodHandles.size(); ++i)
        {
            result->set(static_cast<int>(i), methodHandles[i]);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getMethodReturnType(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getMethodReturnType");
        int64_t methodHandle = extractInt(args[0], "__reflect_getMethodReturnType", "methodHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto methodInfo = registry.getMethod(methodHandle);
        if (!methodInfo.method)
        {
            throw errors::RuntimeException("Invalid method handle");
        }

        return valueTypeToTypeName(methodInfo.method->getReturnType());
    }

    Value ReflectionNatives::__reflect_getMethodParamTypes(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getMethodParamTypes");
        int64_t methodHandle = extractInt(args[0], "__reflect_getMethodParamTypes", "methodHandle");

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
            result->set(static_cast<int>(i - startIndex), valueTypeToTypeName(params[i].second.basicType));
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getMethodParamCount(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getMethodParamCount");
        int64_t methodHandle = extractInt(args[0], "__reflect_getMethodParamCount", "methodHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto methodInfo = registry.getMethod(methodHandle);
        if (!methodInfo.method)
        {
            throw errors::RuntimeException("Invalid method handle");
        }

        return static_cast<int>(methodInfo.userParamCount);
    }

    Value ReflectionNatives::__reflect_getMethodDeclaringClass(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getMethodDeclaringClass");
        int64_t classHandle = extractInt(args[0], "__reflect_getMethodDeclaringClass", "classHandle");
        return classHandle;
    }

    Value ReflectionNatives::__reflect_getMethodModifiers(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getMethodModifiers");
        int64_t methodHandle = extractInt(args[0], "__reflect_getMethodModifiers", "methodHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto methodInfo = registry.getMethod(methodHandle);
        if (!methodInfo.method)
        {
            throw errors::RuntimeException("Invalid method handle");
        }

        return methodInfo.method->getModifierFlags();
    }

    Value ReflectionNatives::__reflect_isMethodAsync(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_isMethodAsync");
        int64_t methodHandle = extractInt(args[0], "__reflect_isMethodAsync", "methodHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto methodInfo = registry.getMethod(methodHandle);
        if (!methodInfo.method)
        {
            throw errors::RuntimeException("Invalid method handle");
        }

        return methodInfo.method->getIsAsync();
    }

    Value ReflectionNatives::__reflect_isMethodGeneric(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_isMethodGeneric");
        int64_t methodHandle = extractInt(args[0], "__reflect_isMethodGeneric", "methodHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto methodInfo = registry.getMethod(methodHandle);
        if (!methodInfo.method)
        {
            throw errors::RuntimeException("Invalid method handle");
        }

        return methodInfo.method->isGeneric();
    }

    Value ReflectionNatives::__reflect_invokeMethod(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 4, "__reflect_invokeMethod");

        if (!ctx.vm) {
            throw errors::RuntimeException("VM not initialized for reflection method invocation");
        }

        auto instance = extractObject(args[0], "__reflect_invokeMethod", "instance");
        int64_t methodHandle = extractInt(args[1], "__reflect_invokeMethod", "methodHandle");
        bool accessible = extractBool(args[3], "__reflect_invokeMethod", "accessible");

        auto& registry = ReflectionHandleRegistry::instance();
        auto methodInfo = registry.getMethod(methodHandle);
        if (!methodInfo.method)
        {
            throw errors::RuntimeException("Invalid method handle");
        }

        if (!accessible && methodInfo.method->getAccessModifier() != ast::AccessModifier::PUBLIC)
        {
            throw errors::RuntimeException("Cannot invoke non-public method '" + methodInfo.methodName +
                "' without setting accessible to true");
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
            throw errors::RuntimeException("Expected array for arguments parameter in __reflect_invokeMethod");
        }

        return ctx.vm->invokeMethod(instance, methodInfo.methodName, argsVec);
    }

    Value ReflectionNatives::__reflect_invokeStaticMethod(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 4, "__reflect_invokeStaticMethod");

        if (!ctx.vm) {
            throw errors::RuntimeException("VM not initialized for reflection static method invocation");
        }

        int64_t classHandle = extractInt(args[0], "__reflect_invokeStaticMethod", "classHandle");
        int64_t methodHandle = extractInt(args[1], "__reflect_invokeStaticMethod", "methodHandle");
        bool accessible = extractBool(args[3], "__reflect_invokeStaticMethod", "accessible");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        auto methodInfo = registry.getMethod(methodHandle);
        if (!methodInfo.method)
        {
            throw errors::RuntimeException("Invalid method handle");
        }

        if (methodInfo.classHandle != classHandle)
        {
            throw errors::RuntimeException("Method '" + methodInfo.methodName +
                "' does not belong to class '" + classDef->getName() + "'");
        }

        if (!accessible && methodInfo.method->getAccessModifier() != ast::AccessModifier::PUBLIC)
        {
            throw errors::RuntimeException("Cannot invoke non-public static method '" + methodInfo.methodName +
                "' without setting accessible to true");
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
            throw errors::RuntimeException("Expected array for arguments parameter in __reflect_invokeStaticMethod");
        }

        return ctx.vm->invokeStaticMethod(classDef->getName(), methodInfo.methodName, argsVec);
    }

    Value ReflectionNatives::__reflect_getMethodName(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getMethodName");
        int64_t methodHandle = extractInt(args[0], "__reflect_getMethodName", "methodHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto methodInfo = registry.getMethod(methodHandle);
        if (!methodInfo.method)
        {
            throw errors::RuntimeException("Invalid method handle");
        }

        return methodInfo.methodName;
    }


} // namespace reflection






