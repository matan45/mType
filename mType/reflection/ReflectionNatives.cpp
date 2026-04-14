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

    // Static member initialization
    std::shared_ptr<environment::Environment> ReflectionNatives::currentEnvironment = nullptr;
    std::shared_ptr<vm::runtime::VirtualMachine> ReflectionNatives::currentVM = nullptr;

    void ReflectionNatives::setEnvironment(std::shared_ptr<environment::Environment> env)
    {
        currentEnvironment = env;
    }

    void ReflectionNatives::setVM(std::shared_ptr<vm::runtime::VirtualMachine> vm)
    {
        currentVM = std::move(vm);
    }

    void ReflectionNatives::registerAll(std::shared_ptr<environment::Environment> env)
    {
        currentEnvironment = env;
        auto nativeRegistry = env->getNativeRegistry();

        // Class reflection
        nativeRegistry->registerNativeFunction("__reflect_forName", __reflect_forName);
        nativeRegistry->registerNativeFunction("__reflect_getSimpleName", __reflect_getSimpleName);
        nativeRegistry->registerNativeFunction("__reflect_getSuperclass", __reflect_getSuperclass);
        nativeRegistry->registerNativeFunction("__reflect_getInterfaces", __reflect_getInterfaces);
        nativeRegistry->registerNativeFunction("__reflect_isInterface", __reflect_isInterface);
        nativeRegistry->registerNativeFunction("__reflect_isAbstract", __reflect_isAbstract);
        nativeRegistry->registerNativeFunction("__reflect_isFinal", __reflect_isFinal);
        nativeRegistry->registerNativeFunction("__reflect_isInstance", __reflect_isInstance);
        nativeRegistry->registerNativeFunction("__reflect_isAssignableFrom", __reflect_isAssignableFrom);
        nativeRegistry->registerNativeFunction("__reflect_newInstance", __reflect_newInstance);
        nativeRegistry->registerNativeFunction("__reflect_isGenericClass", __reflect_isGenericClass);
        nativeRegistry->registerNativeFunction("__reflect_getTypeParameters", __reflect_getTypeParameters);
        nativeRegistry->registerNativeFunction("__reflect_getTypeArguments", __reflect_getTypeArguments);
        nativeRegistry->registerNativeFunction("__reflect_getClassModifiers", __reflect_getClassModifiers);
        nativeRegistry->registerNativeFunction("__reflect_getName", __reflect_getName);
        nativeRegistry->registerNativeFunction("__reflect_getRawName", __reflect_getRawName);

        // Field reflection
        nativeRegistry->registerNativeFunction("__reflect_getField", __reflect_getField);
        nativeRegistry->registerNativeFunction("__reflect_getFields", __reflect_getFields);
        nativeRegistry->registerNativeFunction("__reflect_getFieldType", __reflect_getFieldType);
        nativeRegistry->registerNativeFunction("__reflect_getFieldDeclaringClass", __reflect_getFieldDeclaringClass);
        nativeRegistry->registerNativeFunction("__reflect_getFieldModifiers", __reflect_getFieldModifiers);
        nativeRegistry->registerNativeFunction("__reflect_getFieldValue", __reflect_getFieldValue);
        nativeRegistry->registerNativeFunction("__reflect_setFieldValue", __reflect_setFieldValue);
        nativeRegistry->registerNativeFunction("__reflect_getStaticFieldValue", __reflect_getStaticFieldValue);
        nativeRegistry->registerNativeFunction("__reflect_setStaticFieldValue", __reflect_setStaticFieldValue);
        nativeRegistry->registerNativeFunction("__reflect_getFieldName", __reflect_getFieldName);

        // Method reflection
        nativeRegistry->registerNativeFunction("__reflect_getMethod", __reflect_getMethod);
        nativeRegistry->registerNativeFunction("__reflect_getMethods", __reflect_getMethods);
        nativeRegistry->registerNativeFunction("__reflect_getMethodReturnType", __reflect_getMethodReturnType);
        nativeRegistry->registerNativeFunction("__reflect_getMethodParamTypes", __reflect_getMethodParamTypes);
        nativeRegistry->registerNativeFunction("__reflect_getMethodParamCount", __reflect_getMethodParamCount);
        nativeRegistry->registerNativeFunction("__reflect_getMethodDeclaringClass", __reflect_getMethodDeclaringClass);
        nativeRegistry->registerNativeFunction("__reflect_getMethodModifiers", __reflect_getMethodModifiers);
        nativeRegistry->registerNativeFunction("__reflect_isMethodAsync", __reflect_isMethodAsync);
        nativeRegistry->registerNativeFunction("__reflect_isMethodGeneric", __reflect_isMethodGeneric);
        nativeRegistry->registerNativeFunction("__reflect_invokeMethod", __reflect_invokeMethod);
        nativeRegistry->registerNativeFunction("__reflect_invokeStaticMethod", __reflect_invokeStaticMethod);
        nativeRegistry->registerNativeFunction("__reflect_getMethodName", __reflect_getMethodName);

        // Constructor reflection
        nativeRegistry->registerNativeFunction("__reflect_getConstructor", __reflect_getConstructor);
        nativeRegistry->registerNativeFunction("__reflect_getConstructors", __reflect_getConstructors);
        nativeRegistry->registerNativeFunction("__reflect_getConstructorParamTypes", __reflect_getConstructorParamTypes);
        nativeRegistry->registerNativeFunction("__reflect_getConstructorParamCount", __reflect_getConstructorParamCount);
        nativeRegistry->registerNativeFunction("__reflect_getConstructorModifiers", __reflect_getConstructorModifiers);
        nativeRegistry->registerNativeFunction("__reflect_newInstanceWithArgs", __reflect_newInstanceWithArgs);
        nativeRegistry->registerNativeFunction("__reflect_getConstructorDeclaringClass", __reflect_getConstructorDeclaringClass);

        // Annotation reflection
        nativeRegistry->registerNativeFunction("__reflect_getClassAnnotations", __reflect_getClassAnnotations);
        nativeRegistry->registerNativeFunction("__reflect_getClassAnnotation", __reflect_getClassAnnotation);
        nativeRegistry->registerNativeFunction("__reflect_hasClassAnnotation", __reflect_hasClassAnnotation);
        nativeRegistry->registerNativeFunction("__reflect_getMethodAnnotations", __reflect_getMethodAnnotations);
        nativeRegistry->registerNativeFunction("__reflect_getFieldAnnotations", __reflect_getFieldAnnotations);
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationParam", __reflect_getAnnotationParam);
        nativeRegistry->registerNativeFunction("__reflect_hasAnnotationParam", __reflect_hasAnnotationParam);
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationParams", __reflect_getAnnotationParams);

        // MYT-108 typed annotation parameter accessors
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationInt",     __reflect_getAnnotationInt);
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationFloat",   __reflect_getAnnotationFloat);
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationBool",    __reflect_getAnnotationBool);
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationString",  __reflect_getAnnotationString);
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationClass",   __reflect_getAnnotationClass);
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationClassArray", __reflect_getAnnotationClassArray);
        nativeRegistry->registerNativeFunction("__reflect_isAnnotationParamNull", __reflect_isAnnotationParamNull);
        nativeRegistry->registerNativeFunction("__reflect_getMethodAnnotation",  __reflect_getMethodAnnotation);
        nativeRegistry->registerNativeFunction("__reflect_hasMethodAnnotation",  __reflect_hasMethodAnnotation);
        nativeRegistry->registerNativeFunction("__reflect_getFieldAnnotation",   __reflect_getFieldAnnotation);
        nativeRegistry->registerNativeFunction("__reflect_hasFieldAnnotation",   __reflect_hasFieldAnnotation);
        // MYT-109: constructor-annotation reflection + meta-annotation query
        nativeRegistry->registerNativeFunction("__reflect_getConstructorAnnotations", __reflect_getConstructorAnnotations);
        nativeRegistry->registerNativeFunction("__reflect_getConstructorAnnotation",  __reflect_getConstructorAnnotation);
        nativeRegistry->registerNativeFunction("__reflect_hasConstructorAnnotation",  __reflect_hasConstructorAnnotation);
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationMeta",         __reflect_getAnnotationMeta);
        nativeRegistry->registerNativeFunction("__reflect_getAnnotationObject",  __reflect_getAnnotationObject);

        // Parameter reflection
        nativeRegistry->registerNativeFunction("__reflect_getMethodParameters", __reflect_getMethodParameters);
        nativeRegistry->registerNativeFunction("__reflect_getConstructorParameters", __reflect_getConstructorParameters);
        nativeRegistry->registerNativeFunction("__reflect_getParameterType", __reflect_getParameterType);
    }

    // ========== Helper Methods ==========

    void ReflectionNatives::validateArgCount(const std::vector<Value>& args, size_t expected, const std::string& funcName)
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
        if (!std::holds_alternative<int64_t>(arg))
        {
            throw errors::RuntimeException(funcName + " requires " + paramName + " to be an int");
        }
        return std::get<int64_t>(arg);
    }

    const std::string& ReflectionNatives::extractString(const Value& arg, const std::string& funcName, const std::string& paramName)
    {
        if (std::holds_alternative<std::string>(arg))
        {
            return std::get<std::string>(arg);
        }
        if (std::holds_alternative<InternedString>(arg))
        {
            return std::get<InternedString>(arg).getString();
        }
        throw errors::RuntimeException(funcName + " requires " + paramName + " to be a string");
    }

    bool ReflectionNatives::extractBool(const Value& arg, const std::string& funcName, const std::string& paramName)
    {
        if (!std::holds_alternative<bool>(arg))
        {
            throw errors::RuntimeException(funcName + " requires " + paramName + " to be a bool");
        }
        return std::get<bool>(arg);
    }

    std::shared_ptr<ObjectInstance> ReflectionNatives::extractObject(
        const Value& arg, const std::string& funcName, const std::string& paramName)
    {
        if (!std::holds_alternative<std::shared_ptr<ObjectInstance>>(arg))
        {
            throw errors::RuntimeException(funcName + " requires " + paramName + " to be an object");
        }
        return std::get<std::shared_ptr<ObjectInstance>>(arg);
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
        // Reset currentEnvironment to avoid static destruction order issues
        // When program exits, this prevents the shared_ptr from trying to
        // destroy Environment objects that may depend on other static objects
        currentEnvironment.reset();
        currentVM.reset();
    }

} // namespace reflection
