#include "ReflectionNatives.hpp"
#include "ReflectionHandle.hpp"
#include "../errors/RuntimeException.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../value/NativeArray.hpp"
#include "../value/InternedString.hpp"

namespace reflection
{
    using namespace value;
    using namespace runtimeTypes::klass;

    // Static member initialization
    std::shared_ptr<environment::Environment> ReflectionNatives::currentEnvironment = nullptr;

    void ReflectionNatives::setEnvironment(std::shared_ptr<environment::Environment> env)
    {
        currentEnvironment = env;
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
        nativeRegistry->registerNativeFunction("__reflect_getClassModifiers", __reflect_getClassModifiers);

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

        // Parameter reflection
        nativeRegistry->registerNativeFunction("__reflect_getMethodParameters", __reflect_getMethodParameters);
        nativeRegistry->registerNativeFunction("__reflect_getConstructorParameters", __reflect_getConstructorParameters);
        nativeRegistry->registerNativeFunction("__reflect_getParameterType", __reflect_getParameterType);

        // Bitwise operations (for Modifier class)
        nativeRegistry->registerNativeFunction("__bitwise_and", __bitwise_and);
        nativeRegistry->registerNativeFunction("__bitwise_or", __bitwise_or);
        nativeRegistry->registerNativeFunction("__bitwise_xor", __bitwise_xor);
        nativeRegistry->registerNativeFunction("__bitwise_not", __bitwise_not);
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

    int ReflectionNatives::extractInt(const Value& arg, const std::string& funcName, const std::string& paramName)
    {
        if (!std::holds_alternative<int>(arg))
        {
            throw errors::RuntimeException(funcName + " requires " + paramName + " to be an int");
        }
        return std::get<int>(arg);
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

    // ========== Class Reflection Implementations ==========

    Value ReflectionNatives::__reflect_forName(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_forName");
        std::string className = extractString(args[0], "__reflect_forName", "className");

        if (!currentEnvironment)
        {
            throw errors::RuntimeException("Reflection environment not initialized");
        }

        auto classDef = currentEnvironment->findClass(className);
        if (!classDef)
        {
            throw errors::RuntimeException("Class not found: " + className);
        }

        auto& registry = ReflectionHandleRegistry::instance();
        int64_t handle = registry.getOrCreateClassHandle(classDef);

        return static_cast<int>(handle);
    }

    Value ReflectionNatives::__reflect_getSimpleName(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getSimpleName");
        int classHandle = extractInt(args[0], "__reflect_getSimpleName", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        return classDef->getName();
    }

    Value ReflectionNatives::__reflect_getSuperclass(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getSuperclass");
        int classHandle = extractInt(args[0], "__reflect_getSuperclass", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        auto parentClass = classDef->getParentClass();
        if (!parentClass)
        {
            return 0; // No parent class
        }

        int64_t parentHandle = registry.getOrCreateClassHandle(parentClass);
        return static_cast<int>(parentHandle);
    }

    Value ReflectionNatives::__reflect_getInterfaces(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getInterfaces");
        int classHandle = extractInt(args[0], "__reflect_getInterfaces", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        const auto& interfaces = classDef->getImplementedInterfaces();

        // Create an array of interface names (as strings for now)
        auto result = std::make_shared<NativeArray>(interfaces.size(), ValueType::STRING);
        for (size_t i = 0; i < interfaces.size(); ++i)
        {
            result->set(static_cast<int>(i), interfaces[i]);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_isInterface(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_isInterface");
        // Classes are never interfaces in the current implementation
        // Interface reflection would need InterfaceDefinition
        return false;
    }

    Value ReflectionNatives::__reflect_isAbstract(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_isAbstract");
        int classHandle = extractInt(args[0], "__reflect_isAbstract", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        return classDef->isAbstract();
    }

    Value ReflectionNatives::__reflect_isFinal(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_isFinal");
        int classHandle = extractInt(args[0], "__reflect_isFinal", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        return classDef->isFinal();
    }

    Value ReflectionNatives::__reflect_isInstance(const std::vector<Value>& args)
    {
        validateArgCount(args, 2, "__reflect_isInstance");
        int classHandle = extractInt(args[0], "__reflect_isInstance", "classHandle");

        // Handle null check
        if (std::holds_alternative<std::monostate>(args[1]) || std::holds_alternative<nullptr_t>(args[1]))
        {
            return false;
        }

        if (!std::holds_alternative<std::shared_ptr<ObjectInstance>>(args[1]))
        {
            return false;
        }

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        auto instance = std::get<std::shared_ptr<ObjectInstance>>(args[1]);
        return instance->isInstanceOf(classDef->getName());
    }

    Value ReflectionNatives::__reflect_isAssignableFrom(const std::vector<Value>& args)
    {
        validateArgCount(args, 2, "__reflect_isAssignableFrom");
        int thisHandle = extractInt(args[0], "__reflect_isAssignableFrom", "thisClassHandle");
        int otherHandle = extractInt(args[1], "__reflect_isAssignableFrom", "otherClassHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto thisClass = registry.getClass(thisHandle);
        auto otherClass = registry.getClass(otherHandle);

        if (!thisClass || !otherClass)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        // Check if otherClass is same as or subclass of thisClass
        if (thisClass->getName() == otherClass->getName())
        {
            return true;
        }

        return otherClass->isSubclassOf(thisClass->getName());
    }

    Value ReflectionNatives::__reflect_newInstance(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_newInstance");
        int classHandle = extractInt(args[0], "__reflect_newInstance", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        if (classDef->isAbstract())
        {
            throw errors::RuntimeException("Cannot instantiate abstract class: " + classDef->getName());
        }

        // Create new instance with default values
        auto instance = std::make_shared<ObjectInstance>(classDef);

        // Initialize fields with default values
        for (const auto& [name, fieldDef] : classDef->getInstanceFields())
        {
            instance->setField(name, fieldDef->getValue());
        }

        return instance;
    }

    Value ReflectionNatives::__reflect_isGenericClass(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_isGenericClass");
        int classHandle = extractInt(args[0], "__reflect_isGenericClass", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        return classDef->isGeneric();
    }

    Value ReflectionNatives::__reflect_getTypeParameters(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getTypeParameters");
        int classHandle = extractInt(args[0], "__reflect_getTypeParameters", "classHandle");

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

    Value ReflectionNatives::__reflect_getClassModifiers(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getClassModifiers");
        int classHandle = extractInt(args[0], "__reflect_getClassModifiers", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        return classDef->getModifierFlags();
    }

    // ========== Field Reflection Implementations ==========

    Value ReflectionNatives::__reflect_getField(const std::vector<Value>& args)
    {
        validateArgCount(args, 3, "__reflect_getField");
        int classHandle = extractInt(args[0], "__reflect_getField", "classHandle");
        std::string fieldName = extractString(args[1], "__reflect_getField", "fieldName");
        bool declaredOnly = extractBool(args[2], "__reflect_getField", "declaredOnly");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto classDef = handleRegistry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
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
        return static_cast<int>(fieldHandle);
    }

    Value ReflectionNatives::__reflect_getFields(const std::vector<Value>& args)
    {
        validateArgCount(args, 2, "__reflect_getFields");
        int classHandle = extractInt(args[0], "__reflect_getFields", "classHandle");
        bool declaredOnly = extractBool(args[1], "__reflect_getFields", "declaredOnly");

        auto& handleRegistry = ReflectionHandleRegistry::instance();
        auto classDef = handleRegistry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        std::vector<int> fieldHandles;

        // Collect instance fields
        for (const auto& [name, fieldDef] : classDef->getInstanceFields())
        {
            if (declaredOnly || fieldDef->getAccessModifier() == ast::AccessModifier::PUBLIC)
            {
                int64_t handle = handleRegistry.registerField(fieldDef, classHandle, name);
                fieldHandles.push_back(static_cast<int>(handle));
            }
        }

        // Collect static fields
        for (const auto& [name, fieldDef] : classDef->getStaticFields())
        {
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

    Value ReflectionNatives::__reflect_getFieldType(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getFieldType");
        int fieldHandle = extractInt(args[0], "__reflect_getFieldType", "fieldHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto fieldInfo = registry.getField(fieldHandle);
        if (!fieldInfo.field)
        {
            throw errors::RuntimeException("Invalid field handle");
        }

        // Return type name as string
        return valueTypeToTypeName(fieldInfo.field->getType());
    }

    Value ReflectionNatives::__reflect_getFieldDeclaringClass(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getFieldDeclaringClass");
        int classHandle = extractInt(args[0], "__reflect_getFieldDeclaringClass", "classHandle");

        // The classHandle is already the declaring class handle
        return classHandle;
    }

    Value ReflectionNatives::__reflect_getFieldModifiers(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getFieldModifiers");
        int fieldHandle = extractInt(args[0], "__reflect_getFieldModifiers", "fieldHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto fieldInfo = registry.getField(fieldHandle);
        if (!fieldInfo.field)
        {
            throw errors::RuntimeException("Invalid field handle");
        }

        return fieldInfo.field->getModifierFlags();
    }

    Value ReflectionNatives::__reflect_getFieldValue(const std::vector<Value>& args)
    {
        validateArgCount(args, 3, "__reflect_getFieldValue");
        auto instance = extractObject(args[0], "__reflect_getFieldValue", "instance");
        int fieldHandle = extractInt(args[1], "__reflect_getFieldValue", "fieldHandle");
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

    Value ReflectionNatives::__reflect_setFieldValue(const std::vector<Value>& args)
    {
        validateArgCount(args, 4, "__reflect_setFieldValue");
        auto instance = extractObject(args[0], "__reflect_setFieldValue", "instance");
        int fieldHandle = extractInt(args[1], "__reflect_setFieldValue", "fieldHandle");
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

    Value ReflectionNatives::__reflect_getStaticFieldValue(const std::vector<Value>& args)
    {
        validateArgCount(args, 2, "__reflect_getStaticFieldValue");
        int classHandle = extractInt(args[0], "__reflect_getStaticFieldValue", "classHandle");
        int fieldHandle = extractInt(args[1], "__reflect_getStaticFieldValue", "fieldHandle");

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

    Value ReflectionNatives::__reflect_setStaticFieldValue(const std::vector<Value>& args)
    {
        validateArgCount(args, 3, "__reflect_setStaticFieldValue");
        int classHandle = extractInt(args[0], "__reflect_setStaticFieldValue", "classHandle");
        int fieldHandle = extractInt(args[1], "__reflect_setStaticFieldValue", "fieldHandle");
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

    Value ReflectionNatives::__reflect_getFieldName(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getFieldName");
        int fieldHandle = extractInt(args[0], "__reflect_getFieldName", "fieldHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto fieldInfo = registry.getField(fieldHandle);
        if (!fieldInfo.field)
        {
            throw errors::RuntimeException("Invalid field handle");
        }

        return fieldInfo.fieldName;
    }

    // ========== Method Reflection Implementations ==========

    Value ReflectionNatives::__reflect_getMethod(const std::vector<Value>& args)
    {
        validateArgCount(args, 4, "__reflect_getMethod");
        int classHandle = extractInt(args[0], "__reflect_getMethod", "classHandle");
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
        int classHandle = extractInt(args[0], "__reflect_getMethods", "classHandle");
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
        int methodHandle = extractInt(args[0], "__reflect_getMethodReturnType", "methodHandle");

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
        int methodHandle = extractInt(args[0], "__reflect_getMethodParamTypes", "methodHandle");

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
        int methodHandle = extractInt(args[0], "__reflect_getMethodParamCount", "methodHandle");

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
        int classHandle = extractInt(args[0], "__reflect_getMethodDeclaringClass", "classHandle");
        return classHandle;
    }

    Value ReflectionNatives::__reflect_getMethodModifiers(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getMethodModifiers");
        int methodHandle = extractInt(args[0], "__reflect_getMethodModifiers", "methodHandle");

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
        int methodHandle = extractInt(args[0], "__reflect_isMethodAsync", "methodHandle");

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
        int methodHandle = extractInt(args[0], "__reflect_isMethodGeneric", "methodHandle");

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
        // TODO: Implement full method invocation via VM
        // This requires integration with the VirtualMachine
        throw errors::RuntimeException("Method invocation via reflection not yet implemented");
    }

    Value ReflectionNatives::__reflect_invokeStaticMethod(const std::vector<Value>& args)
    {
        // TODO: Implement full static method invocation via VM
        throw errors::RuntimeException("Static method invocation via reflection not yet implemented");
    }

    Value ReflectionNatives::__reflect_getMethodName(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getMethodName");
        int methodHandle = extractInt(args[0], "__reflect_getMethodName", "methodHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto methodInfo = registry.getMethod(methodHandle);
        if (!methodInfo.method)
        {
            throw errors::RuntimeException("Invalid method handle");
        }

        return methodInfo.methodName;
    }

    // ========== Constructor Reflection Implementations ==========

    Value ReflectionNatives::__reflect_getConstructor(const std::vector<Value>& args)
    {
        validateArgCount(args, 3, "__reflect_getConstructor");
        int classHandle = extractInt(args[0], "__reflect_getConstructor", "classHandle");
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
        int classHandle = extractInt(args[0], "__reflect_getConstructors", "classHandle");
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
        int ctorHandle = extractInt(args[0], "__reflect_getConstructorParamTypes", "ctorHandle");

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
        int ctorHandle = extractInt(args[0], "__reflect_getConstructorParamCount", "ctorHandle");

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
        int ctorHandle = extractInt(args[0], "__reflect_getConstructorModifiers", "ctorHandle");

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
        // TODO: Implement full constructor invocation via VM
        throw errors::RuntimeException("Constructor invocation via reflection not yet implemented");
    }

    Value ReflectionNatives::__reflect_getConstructorDeclaringClass(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getConstructorDeclaringClass");
        int classHandle = extractInt(args[0], "__reflect_getConstructorDeclaringClass", "classHandle");
        return classHandle;
    }

    // ========== Annotation Reflection Implementations ==========

    Value ReflectionNatives::__reflect_getClassAnnotations(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_getClassAnnotations");
        int classHandle = extractInt(args[0], "__reflect_getClassAnnotations", "classHandle");

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
        int classHandle = extractInt(args[0], "__reflect_getClassAnnotation", "classHandle");
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
        int classHandle = extractInt(args[0], "__reflect_hasClassAnnotation", "classHandle");
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
        int methodHandle = extractInt(args[0], "__reflect_getMethodAnnotations", "methodHandle");

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
        int annotationHandle = extractInt(args[0], "__reflect_getAnnotationParam", "annotationHandle");
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
        int annotationHandle = extractInt(args[0], "__reflect_hasAnnotationParam", "annotationHandle");
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
        int annotationHandle = extractInt(args[0], "__reflect_getAnnotationParams", "annotationHandle");

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
        int methodHandle = extractInt(args[0], "__reflect_getMethodParameters", "methodHandle");

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
        int ctorHandle = extractInt(args[0], "__reflect_getConstructorParameters", "ctorHandle");

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
        int typeHandle = extractInt(args[0], "__reflect_getParameterType", "typeHandle");

        // typeHandle is actually a class handle in this context
        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(typeHandle);
        if (classDef)
        {
            return classDef->getName();
        }

        return "unknown";
    }

    // ========== Bitwise Operations (for Modifier class) ==========

    Value ReflectionNatives::__bitwise_and(const std::vector<Value>& args)
    {
        validateArgCount(args, 2, "__bitwise_and");
        int a = extractInt(args[0], "__bitwise_and", "a");
        int b = extractInt(args[1], "__bitwise_and", "b");
        return a & b;
    }

    Value ReflectionNatives::__bitwise_or(const std::vector<Value>& args)
    {
        validateArgCount(args, 2, "__bitwise_or");
        int a = extractInt(args[0], "__bitwise_or", "a");
        int b = extractInt(args[1], "__bitwise_or", "b");
        return a | b;
    }

    Value ReflectionNatives::__bitwise_xor(const std::vector<Value>& args)
    {
        validateArgCount(args, 2, "__bitwise_xor");
        int a = extractInt(args[0], "__bitwise_xor", "a");
        int b = extractInt(args[1], "__bitwise_xor", "b");
        return a ^ b;
    }

    Value ReflectionNatives::__bitwise_not(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__bitwise_not");
        int a = extractInt(args[0], "__bitwise_not", "a");
        return ~a;
    }

    void ReflectionNatives::cleanup()
    {
        // Clear the reflection handle registry to avoid stale handles between tests
        ReflectionHandleRegistry::instance().clear();
        // Reset currentEnvironment to avoid static destruction order issues
        // When program exits, this prevents the shared_ptr from trying to
        // destroy Environment objects that may depend on other static objects
        currentEnvironment.reset();
    }

} // namespace reflection
