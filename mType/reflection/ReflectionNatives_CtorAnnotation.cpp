// MYT-126: walled off under flag-on — variant accessors not migrated.
#ifndef MTYPE_TAGGED_VALUE
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


    Value ReflectionNatives::__reflect_getConstructor(const std::vector<Value>& args)
    {
        validateArgCount(args, 3, "__reflect_getConstructor");
        int64_t classHandle = extractInt(args[0], "__reflect_getConstructor", "classHandle");
        // args[1] is paramTypes array
        bool declaredOnly = extractBool(args[2], "__reflect_getConstructor", "declaredOnly");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto classDef = handleRegistry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        size_t paramCount = 0;
        if (std::holds_alternative<std::shared_ptr<NativeArray>>(args[1]))
        {
            auto paramArray = std::get<std::shared_ptr<NativeArray>>(args[1]);
            paramCount = paramArray->size();
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

        // Find the shared_ptr for this constructor
        const auto& constructors = classDef->getConstructors();
        for (size_t i = 0; i < constructors.size(); ++i)
        {
            if (constructors[i].get() == ctorDef)
            {
                int64_t ctorHandle = handleRegistry.registerConstructor(constructors[i], classHandle, static_cast<int>(i));
                return static_cast<int>(ctorHandle);
            }
        }

        throw errors::RuntimeException("Constructor registration failed");
    }

    Value ReflectionNatives::__reflect_getConstructors(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_getConstructorParamTypes(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_getConstructorParamCount(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_getConstructorModifiers(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_newInstanceWithArgs(const std::vector<Value>& args)
    {
        validateArgCount(args, 4, "__reflect_newInstanceWithArgs");

        if (!currentVM)
        {
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
            throw errors::RuntimeException("Expected array for arguments parameter in __reflect_newInstanceWithArgs");
        }

        return currentVM->createObject(classDef->getName(), argsVec);
    }

    Value ReflectionNatives::__reflect_getConstructorDeclaringClass(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getConstructorDeclaringClass");
        int64_t classHandle = extractInt(args[0], "__reflect_getConstructorDeclaringClass", "classHandle");
        return classHandle;
    }

    // ========== Annotation Reflection Implementations ==========

    Value ReflectionNatives::__reflect_getClassAnnotations(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_getClassAnnotation(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_hasClassAnnotation(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_getMethodAnnotations(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_getFieldAnnotations(const std::vector<Value>& args)
    {
        // Fields don't have annotations in current implementation
        auto result = std::make_shared<NativeArray>(0, ValueType::INT);
        return result;
    }

    Value ReflectionNatives::__reflect_getAnnotationParam(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_hasAnnotationParam(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_getAnnotationParams(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_getMethodParameters(const std::vector<Value>& args)
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

        // Return array of parameter names
        auto result = std::make_shared<NativeArray>(params.size(), ValueType::STRING);
        for (size_t i = 0; i < params.size(); ++i)
        {
            result->set(static_cast<int>(i), params[i].first);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getConstructorParameters(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_getParameterType(const std::vector<Value>& args)
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


} // namespace reflection

#endif
