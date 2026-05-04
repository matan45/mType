#include "ReflectionNatives.hpp"
#include <cstdint>
#include "ReflectionHandle.hpp"
#include "../errors/RuntimeException.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../value/NativeArray.hpp"
#include "../value/InternedString.hpp"
#include "../value/ValueShim.hpp"
#include "../vm/runtime/VirtualMachine.hpp"

namespace reflection
{
    using namespace value;
    using namespace runtimeTypes::klass;

    void ReflectionNatives::registerAll(std::shared_ptr<environment::Environment> env)
    {
        auto nativeRegistry = env->getNativeRegistry();

        // Class reflection
        nativeRegistry->registerNativeFunction("__reflect_forName", { nullptr, __reflect_forName });
        nativeRegistry->registerNativeFunction("__reflect_getSimpleName", { nullptr, __reflect_getSimpleName });
        nativeRegistry->registerNativeFunction("__reflect_getSuperclass", { nullptr, __reflect_getSuperclass });
        nativeRegistry->registerNativeFunction("__reflect_getInterfaces", { nullptr, __reflect_getInterfaces });
        nativeRegistry->registerNativeFunction("__reflect_isInterface", { nullptr, __reflect_isInterface });
        nativeRegistry->registerNativeFunction("__reflect_isAbstract", { nullptr, __reflect_isAbstract });
        nativeRegistry->registerNativeFunction("__reflect_isFinal", { nullptr, __reflect_isFinal });
        nativeRegistry->registerNativeFunction("__reflect_isInstance", { nullptr, __reflect_isInstance });
        nativeRegistry->registerNativeFunction("__reflect_isAssignableFrom", { nullptr, __reflect_isAssignableFrom });
        nativeRegistry->registerNativeFunction("__reflect_newInstance", { nullptr, __reflect_newInstance });
        nativeRegistry->registerNativeFunction("__reflect_isGenericClass", { nullptr, __reflect_isGenericClass });
        nativeRegistry->registerNativeFunction("__reflect_getTypeParameters", { nullptr, __reflect_getTypeParameters });
        nativeRegistry->registerNativeFunction("__reflect_getTypeArguments", { nullptr, __reflect_getTypeArguments });
        nativeRegistry->registerNativeFunction("__reflect_getClassModifiers", { nullptr, __reflect_getClassModifiers });
        nativeRegistry->registerNativeFunction("__reflect_getName", { nullptr, __reflect_getName });
        nativeRegistry->registerNativeFunction("__reflect_getRawName", { nullptr, __reflect_getRawName });

        // Field reflection
        nativeRegistry->registerNativeFunction("__reflect_getField", { nullptr, __reflect_getField });
        nativeRegistry->registerNativeFunction("__reflect_getFields", { nullptr, __reflect_getFields });
        nativeRegistry->registerNativeFunction("__reflect_getFieldType", { nullptr, __reflect_getFieldType });
        nativeRegistry->registerNativeFunction("__reflect_getFieldDeclaringClass", { nullptr, __reflect_getFieldDeclaringClass });
        nativeRegistry->registerNativeFunction("__reflect_getFieldModifiers", { nullptr, __reflect_getFieldModifiers });
        nativeRegistry->registerNativeFunction("__reflect_getFieldValue", { nullptr, __reflect_getFieldValue });
        nativeRegistry->registerNativeFunction("__reflect_setFieldValue", { nullptr, __reflect_setFieldValue });
        nativeRegistry->registerNativeFunction("__reflect_getStaticFieldValue", { nullptr, __reflect_getStaticFieldValue });
        nativeRegistry->registerNativeFunction("__reflect_setStaticFieldValue", { nullptr, __reflect_setStaticFieldValue });
        nativeRegistry->registerNativeFunction("__reflect_getFieldName", { nullptr, __reflect_getFieldName });

        // Method reflection
        nativeRegistry->registerNativeFunction("__reflect_getMethod", { nullptr, __reflect_getMethod });
        nativeRegistry->registerNativeFunction("__reflect_getMethods", { nullptr, __reflect_getMethods });
        nativeRegistry->registerNativeFunction("__reflect_getMethodReturnType", { nullptr, __reflect_getMethodReturnType });
        nativeRegistry->registerNativeFunction("__reflect_getMethodParamTypes", { nullptr, __reflect_getMethodParamTypes });
        nativeRegistry->registerNativeFunction("__reflect_getMethodParamCount", { nullptr, __reflect_getMethodParamCount });
        nativeRegistry->registerNativeFunction("__reflect_getMethodDeclaringClass", { nullptr, __reflect_getMethodDeclaringClass });
        nativeRegistry->registerNativeFunction("__reflect_getMethodModifiers", { nullptr, __reflect_getMethodModifiers });
        nativeRegistry->registerNativeFunction("__reflect_isMethodAsync", { nullptr, __reflect_isMethodAsync });
        nativeRegistry->registerNativeFunction("__reflect_isMethodGeneric", { nullptr, __reflect_isMethodGeneric });
        nativeRegistry->registerNativeFunction("__reflect_invokeMethod", { nullptr, __reflect_invokeMethod });
        nativeRegistry->registerNativeFunction("__reflect_invokeStaticMethod", { nullptr, __reflect_invokeStaticMethod });
        nativeRegistry->registerNativeFunction("__reflect_getMethodName", { nullptr, __reflect_getMethodName });

        // Constructor reflection
        nativeRegistry->registerNativeFunction("__reflect_getConstructor", { nullptr, __reflect_getConstructor });
        nativeRegistry->registerNativeFunction("__reflect_getConstructors", { nullptr, __reflect_getConstructors });
        nativeRegistry->registerNativeFunction("__reflect_getConstructorParamTypes", { nullptr, __reflect_getConstructorParamTypes });
        nativeRegistry->registerNativeFunction("__reflect_getConstructorParamCount", { nullptr, __reflect_getConstructorParamCount });
        nativeRegistry->registerNativeFunction("__reflect_getConstructorModifiers", { nullptr, __reflect_getConstructorModifiers });
        nativeRegistry->registerNativeFunction("__reflect_newInstanceWithArgs", { nullptr, __reflect_newInstanceWithArgs });
        nativeRegistry->registerNativeFunction("__reflect_getConstructorDeclaringClass", { nullptr, __reflect_getConstructorDeclaringClass });

        // Annotation reflection
        nativeRegistry->registerNativeFunction("__reflect_getClassAnnotations", { nullptr, __reflect_getClassAnnotations });
        nativeRegistry->registerNativeFunction("__reflect_getClassAnnotation", { nullptr, __reflect_getClassAnnotation });
        nativeRegistry->registerNativeFunction("__reflect_hasClassAnnotation", { nullptr, __reflect_hasClassAnnotation });
        nativeRegistry->registerNativeFunction("__reflect_getMethodAnnotations", { nullptr, __reflect_getMethodAnnotations });
        nativeRegistry->registerNativeFunction("__reflect_getFieldAnnotations", { nullptr, __reflect_getFieldAnnotations });
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationParam", { nullptr, __reflect_getAnnotationParam });
        nativeRegistry->registerNativeFunction("__reflect_hasAnnotationParam", { nullptr, __reflect_hasAnnotationParam });
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationParams", { nullptr, __reflect_getAnnotationParams });

        // MYT-108 typed annotation parameter accessors
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationInt",     { nullptr, __reflect_getAnnotationInt });
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationFloat",   { nullptr, __reflect_getAnnotationFloat });
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationBool",    { nullptr, __reflect_getAnnotationBool });
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationString",  { nullptr, __reflect_getAnnotationString });
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationClass",   { nullptr, __reflect_getAnnotationClass });
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationClassArray", { nullptr, __reflect_getAnnotationClassArray });
        nativeRegistry->registerNativeFunction("__reflect_isAnnotationParamNull", { nullptr, __reflect_isAnnotationParamNull });
        nativeRegistry->registerNativeFunction("__reflect_getMethodAnnotation",  { nullptr, __reflect_getMethodAnnotation });
        nativeRegistry->registerNativeFunction("__reflect_hasMethodAnnotation",  { nullptr, __reflect_hasMethodAnnotation });
        nativeRegistry->registerNativeFunction("__reflect_getFieldAnnotation",   { nullptr, __reflect_getFieldAnnotation });
        nativeRegistry->registerNativeFunction("__reflect_hasFieldAnnotation",   { nullptr, __reflect_hasFieldAnnotation });
        // MYT-109: constructor-annotation reflection + meta-annotation query
        nativeRegistry->registerNativeFunction("__reflect_getConstructorAnnotations", { nullptr, __reflect_getConstructorAnnotations });
        nativeRegistry->registerNativeFunction("__reflect_getConstructorAnnotation",  { nullptr, __reflect_getConstructorAnnotation });
        nativeRegistry->registerNativeFunction("__reflect_hasConstructorAnnotation",  { nullptr, __reflect_hasConstructorAnnotation });
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationMeta",         { nullptr, __reflect_getAnnotationMeta });
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationObject",  { nullptr, __reflect_getAnnotationObject });

        // Parameter reflection
        nativeRegistry->registerNativeFunction("__reflect_getMethodParameters", { nullptr, __reflect_getMethodParameters });
        nativeRegistry->registerNativeFunction("__reflect_getConstructorParameters", { nullptr, __reflect_getConstructorParameters });
        nativeRegistry->registerNativeFunction("__reflect_getParameterType", { nullptr, __reflect_getParameterType });

        // MYT-110: parameter-level annotation reflection
        nativeRegistry->registerNativeFunction("__reflect_getMethodParameterAnnotations", { nullptr, __reflect_getMethodParameterAnnotations });
        nativeRegistry->registerNativeFunction("__reflect_getMethodParameterAnnotation",  { nullptr, __reflect_getMethodParameterAnnotation });
        nativeRegistry->registerNativeFunction("__reflect_hasMethodParameterAnnotation",  { nullptr, __reflect_hasMethodParameterAnnotation });
        nativeRegistry->registerNativeFunction("__reflect_getConstructorParameterAnnotations", { nullptr, __reflect_getConstructorParameterAnnotations });
        nativeRegistry->registerNativeFunction("__reflect_getConstructorParameterAnnotation",  { nullptr, __reflect_getConstructorParameterAnnotation });
        nativeRegistry->registerNativeFunction("__reflect_hasConstructorParameterAnnotation",  { nullptr, __reflect_hasConstructorParameterAnnotation });
    }

    // ========== Helper Methods ==========

    void ReflectionNatives::validateArgCount(std::span<const Value> args, size_t expected, const std::string& funcName)
    {
        if (args.size() != expected)
        {
            throw errors::RuntimeException(funcName + " requires " + std::to_string(expected) +
                                          " argument" + (expected != 1 ? "s" : "") +
                                          ", got " + std::to_string(args.size()));
        }
    }

    int64_t ReflectionNatives::extractInt(const Value& arg, const std::string& funcName, const std::string& paramName)
    {
        if (!isInt(arg))
        {
            throw errors::RuntimeException(funcName + " requires " + paramName + " to be an int");
        }
        return asInt(arg);
    }

    const std::string& ReflectionNatives::extractString(const Value& arg, const std::string& funcName, const std::string& paramName)
    {
        if (isString(arg))
        {
            return asString(arg);
        }
        if (isInternedString(arg))
        {
            return asInternedString(arg).getString();
        }
        throw errors::RuntimeException(funcName + " requires " + paramName + " to be a string");
    }

    bool ReflectionNatives::extractBool(const Value& arg, const std::string& funcName, const std::string& paramName)
    {
        if (!isBool(arg))
        {
            throw errors::RuntimeException(funcName + " requires " + paramName + " to be a bool");
        }
        return asBool(arg);
    }

    std::shared_ptr<ObjectInstance> ReflectionNatives::extractObject(
        const Value& arg, const std::string& funcName, const std::string& paramName)
    {
        if (!isObject(arg))
        {
            throw errors::RuntimeException(funcName + " requires " + paramName + " to be an object");
        }
        return asObject(arg);
    }

    std::string ReflectionNatives::valueTypeToTypeName(ValueType type)
    {
        switch (type)
        {
            case ValueType::INT: return "int";
            case ValueType::FLOAT: return "float";
            case ValueType::BOOL: return "bool";
            case ValueType::STRING: return "String";
            case ValueType::VOID: return "void";
            case ValueType::OBJECT: return "Object";
            case ValueType::ARRAY: return "Array";
            case ValueType::LAMBDA: return "Lambda";
            case ValueType::NULL_TYPE: return "null";
            default: return "unknown";
        }
    }


    void ReflectionNatives::cleanup()
    {
        // Clear the reflection handle registry to avoid stale handles between tests
        ReflectionHandleRegistry::instance().clear();
    }

} // namespace reflection
