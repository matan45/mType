#include "ReflectionNatives.hpp"
#include "ReflectionHandle.hpp"
#include "../errors/RuntimeException.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../value/NativeArray.hpp"
#include "../value/InternedString.hpp"
#include "../vm/runtime/VirtualMachine.hpp"

namespace reflection
{
    using namespace value;
    using namespace runtimeTypes::klass;


    Value ReflectionNatives::__reflect_getMethod(const std::vector<Value>& args)
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
        if (std::holds_alternative<std::shared_ptr<NativeArray>>(args[2]))
        {
            auto paramArray = std::get<std::shared_ptr<NativeArray>>(args[2]);
            paramCount = paramArray->size();
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
        return static_cast<int>(methodHandle);
    }

    Value ReflectionNatives::__reflect_getMethods(const std::vector<Value>& args)
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

        // Collect instance methods
        for (const auto& [name, overloads] : classDef->getInstanceMethods())
        {
            for (const auto& methodDef : overloads)
            {
                if (declaredOnly || methodDef->getAccessModifier() == ast::AccessModifier::PUBLIC)
                {
                    int64_t handle = handleRegistry.registerMethod(methodDef, classHandle, name);
                    methodHandles.push_back(static_cast<int>(handle));
                }
            }
        }

        // Collect static methods
        for (const auto& [name, overloads] : classDef->getStaticMethods())
        {
            for (const auto& methodDef : overloads)
            {
                if (declaredOnly || methodDef->getAccessModifier() == ast::AccessModifier::PUBLIC)
                {
                    int64_t handle = handleRegistry.registerMethod(methodDef, classHandle, name);
                    methodHandles.push_back(static_cast<int>(handle));
                }
            }
        }

        auto result = std::make_shared<NativeArray>(methodHandles.size(), ValueType::INT);
        for (size_t i = 0; i < methodHandles.size(); ++i)
        {
            result->set(static_cast<int>(i), methodHandles[i]);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getMethodReturnType(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_getMethodParamTypes(const std::vector<Value>& args)
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
        auto result = std::make_shared<NativeArray>(params.size(), ValueType::STRING);

        for (size_t i = 0; i < params.size(); ++i)
        {
            result->set(static_cast<int>(i), valueTypeToTypeName(params[i].second.basicType));
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getMethodParamCount(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getMethodParamCount");
        int64_t methodHandle = extractInt(args[0], "__reflect_getMethodParamCount", "methodHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto methodInfo = registry.getMethod(methodHandle);
        if (!methodInfo.method)
        {
            throw errors::RuntimeException("Invalid method handle");
        }

        return static_cast<int>(methodInfo.method->getParameters().size());
    }

    Value ReflectionNatives::__reflect_getMethodDeclaringClass(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getMethodDeclaringClass");
        int64_t classHandle = extractInt(args[0], "__reflect_getMethodDeclaringClass", "classHandle");
        return classHandle;
    }

    Value ReflectionNatives::__reflect_getMethodModifiers(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_isMethodAsync(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_isMethodGeneric(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_invokeMethod(const std::vector<Value>& args)
    {
        validateArgCount(args, 4, "__reflect_invokeMethod");

        if (!currentVM)
        {
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
        if (std::holds_alternative<std::shared_ptr<NativeArray>>(args[2]))
        {
            auto argArray = std::get<std::shared_ptr<NativeArray>>(args[2]);
            for (size_t i = 0; i < argArray->size(); ++i)
            {
                argsVec.push_back(argArray->get(i));
            }
        }
        else if (!std::holds_alternative<std::monostate>(args[2]) && !std::holds_alternative<nullptr_t>(args[2]))
        {
            throw errors::RuntimeException("Expected array for arguments parameter in __reflect_invokeMethod");
        }

        return currentVM->invokeMethod(instance, methodInfo.methodName, argsVec);
    }

    Value ReflectionNatives::__reflect_invokeStaticMethod(const std::vector<Value>& args)
    {
        validateArgCount(args, 4, "__reflect_invokeStaticMethod");

        if (!currentVM)
        {
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
        if (std::holds_alternative<std::shared_ptr<NativeArray>>(args[2]))
        {
            auto argArray = std::get<std::shared_ptr<NativeArray>>(args[2]);
            for (size_t i = 0; i < argArray->size(); ++i)
            {
                argsVec.push_back(argArray->get(i));
            }
        }
        else if (!std::holds_alternative<std::monostate>(args[2]) && !std::holds_alternative<nullptr_t>(args[2]))
        {
            throw errors::RuntimeException("Expected array for arguments parameter in __reflect_invokeStaticMethod");
        }

        return currentVM->invokeStaticMethod(classDef->getName(), methodInfo.methodName, argsVec);
    }

    Value ReflectionNatives::__reflect_getMethodName(const std::vector<Value>& args)
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
