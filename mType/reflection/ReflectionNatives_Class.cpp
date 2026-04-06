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
        int64_t classHandle = extractInt(args[0], "__reflect_getSimpleName", "classHandle");

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
        int64_t classHandle = extractInt(args[0], "__reflect_getSuperclass", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        auto parentClass = classDef->getParentClass();
        if (!parentClass)
        {
            return 0;
        }

        int64_t parentHandle = registry.getOrCreateClassHandle(parentClass);
        return static_cast<int>(parentHandle);
    }

    Value ReflectionNatives::__reflect_getInterfaces(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_isInterface(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_isInterface");
        return false;
    }

    Value ReflectionNatives::__reflect_isAbstract(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_isAbstract");
        int64_t classHandle = extractInt(args[0], "__reflect_isAbstract", "classHandle");

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
        int64_t classHandle = extractInt(args[0], "__reflect_isFinal", "classHandle");

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
        int64_t classHandle = extractInt(args[0], "__reflect_isInstance", "classHandle");

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
        int64_t thisHandle = extractInt(args[0], "__reflect_isAssignableFrom", "thisClassHandle");
        int64_t otherHandle = extractInt(args[1], "__reflect_isAssignableFrom", "otherClassHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto thisClass = registry.getClass(thisHandle);
        auto otherClass = registry.getClass(otherHandle);

        if (!thisClass || !otherClass)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        if (thisClass->getName() == otherClass->getName())
        {
            return true;
        }

        return otherClass->isSubclassOf(thisClass->getName());
    }

    Value ReflectionNatives::__reflect_newInstance(const std::vector<Value>& args)
    {
        validateArgCount(args, 1, "__reflect_newInstance");
        int64_t classHandle = extractInt(args[0], "__reflect_newInstance", "classHandle");

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

        auto instance = std::make_shared<ObjectInstance>(classDef);
        for (const auto& [name, fieldDef] : classDef->getInstanceFields())
        {
            instance->setField(name, fieldDef->getValue());
        }

        return instance;
    }

    Value ReflectionNatives::__reflect_isGenericClass(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_getTypeParameters(const std::vector<Value>& args)
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

    Value ReflectionNatives::__reflect_getClassModifiers(const std::vector<Value>& args)
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

} // namespace reflection
