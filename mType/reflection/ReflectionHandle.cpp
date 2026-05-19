#include "ReflectionHandle.hpp"
#include <cstddef>
#include <stdexcept>
#include "../types/ReifiedTypeRegistry.hpp"

namespace reflection
{
    namespace
    {
        inline size_t hashCombine(size_t seed, size_t value)
        {
            return seed ^ (value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2));
        }
    }

    size_t FieldLookupKeyHash::operator()(const FieldLookupKey& key) const
    {
        size_t seed = std::hash<int64_t>{}(key.classHandle);
        seed = hashCombine(seed, std::hash<bool>{}(key.declaredOnly));
        return hashCombine(seed, std::hash<std::string>{}(key.fieldName));
    }

    size_t MethodLookupKeyHash::operator()(const MethodLookupKey& key) const
    {
        size_t seed = std::hash<int64_t>{}(key.classHandle);
        seed = hashCombine(seed, std::hash<size_t>{}(key.paramCount));
        seed = hashCombine(seed, std::hash<bool>{}(key.declaredOnly));
        return hashCombine(seed, std::hash<std::string>{}(key.methodName));
    }

    size_t ConstructorLookupKeyHash::operator()(const ConstructorLookupKey& key) const
    {
        size_t seed = std::hash<int64_t>{}(key.classHandle);
        seed = hashCombine(seed, std::hash<size_t>{}(key.paramCount));
        return hashCombine(seed, std::hash<bool>{}(key.declaredOnly));
    }

    std::string ReflectionHandleRegistry::makeFieldKey(int64_t classHandle, const std::string& fieldName)
    {
        return std::to_string(classHandle) + ":" + fieldName;
    }

    std::string ReflectionHandleRegistry::makeMethodKey(int64_t classHandle, const std::string& methodName,
                                                        const std::vector<std::string>& paramTypes)
    {
        // Length-prefixed encoding avoids delimiter collisions with generic
        // types like Foo<A,B>. Format: "classHandle:methodName#count:len1:type1:..."
        std::string key = std::to_string(classHandle) + ":" + methodName + "#" + std::to_string(paramTypes.size());
        for (const auto& paramType : paramTypes)
        {
            key += ":" + std::to_string(paramType.length()) + ":" + paramType;
        }
        return key;
    }

    std::string ReflectionHandleRegistry::makeConstructorKey(int64_t classHandle, int constructorIndex)
    {
        return std::to_string(classHandle) + ":ctor:" + std::to_string(constructorIndex);
    }

    ReflectionHandleRegistry& ReflectionHandleRegistry::instance()
    {
        static ReflectionHandleRegistry instance;
        return instance;
    }

    int64_t ReflectionHandleRegistry::registerClass(
        const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef)
    {
        if (!classDef)
        {
            throw std::invalid_argument("registerClass: classDef cannot be null");
        }

        const std::string& className = classDef->getName();
        auto it = classNameToHandle.find(className);
        if (it != classNameToHandle.end())
        {
            return it->second;
        }

        int64_t handle = nextHandle++;
        classHandles[handle] = classDef;
        classNameToHandle[className] = handle;
        handleTypeMap[handle] = HandleType::CLASS;

        return handle;
    }

    std::shared_ptr<runtimeTypes::klass::ClassDefinition> ReflectionHandleRegistry::getClass(int64_t handle) const
    {
        auto it = classHandles.find(handle);
        if (it != classHandles.end())
        {
            return it->second;
        }
        return nullptr;
    }

    int64_t ReflectionHandleRegistry::findClassHandle(const std::string& className) const
    {
        auto it = classNameToHandle.find(className);
        if (it != classNameToHandle.end())
        {
            return it->second;
        }
        return -1;
    }

    int64_t ReflectionHandleRegistry::getOrCreateClassHandle(
        const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef)
    {
        if (!classDef)
        {
            throw std::invalid_argument("getOrCreateClassHandle: classDef cannot be null");
        }

        int64_t existingHandle = findClassHandle(classDef->getName());
        if (existingHandle != -1)
        {
            return existingHandle;
        }

        return registerClass(classDef);
    }

    int64_t ReflectionHandleRegistry::registerField(const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field,
                                                    int64_t classHandle, const std::string& fieldName)
    {
        if (!field)
        {
            throw std::invalid_argument("registerField: field cannot be null");
        }

        std::string key = makeFieldKey(classHandle, fieldName);
        auto it = fieldKeyToHandle.find(key);
        if (it != fieldKeyToHandle.end())
        {
            return it->second;
        }

        int64_t handle = nextHandle++;
        fieldHandles[handle] = FieldHandleInfo{field, classHandle, fieldName};
        fieldKeyToHandle[key] = handle;
        handleTypeMap[handle] = HandleType::FIELD;

        return handle;
    }

    FieldHandleInfo ReflectionHandleRegistry::getField(int64_t handle) const
    {
        auto it = fieldHandles.find(handle);
        if (it != fieldHandles.end())
        {
            return it->second;
        }
        return FieldHandleInfo{nullptr, -1, ""};
    }

    int64_t ReflectionHandleRegistry::registerMethod(
        const std::shared_ptr<runtimeTypes::klass::MethodDefinition>& method,
        int64_t classHandle, const std::string& methodName)
    {
        if (!method)
        {
            throw std::invalid_argument("registerMethod: method cannot be null");
        }

        std::vector<std::string> paramTypes;
        for (const auto& param : method->getParameters())
        {
            paramTypes.push_back(param.second.toString());
        }

        std::string key = makeMethodKey(classHandle, methodName, paramTypes);
        auto it = methodKeyToHandle.find(key);
        if (it != methodKeyToHandle.end())
        {
            return it->second;
        }

        int64_t handle = nextHandle++;
        size_t userParamCount = method->getParameters().size();
        if (!method->isStatic() && userParamCount > 0)
        {
            --userParamCount;
        }
        methodHandles[handle] = MethodHandleInfo{method, classHandle, methodName, userParamCount};
        methodKeyToHandle[key] = handle;
        handleTypeMap[handle] = HandleType::METHOD;

        return handle;
    }

    MethodHandleInfo ReflectionHandleRegistry::getMethod(int64_t handle) const
    {
        auto it = methodHandles.find(handle);
        if (it != methodHandles.end())
        {
            return it->second;
        }
        return MethodHandleInfo{nullptr, -1, "", 0};
    }

    int64_t ReflectionHandleRegistry::registerConstructor(
        const std::shared_ptr<runtimeTypes::klass::ConstructorDefinition>& constructor,
        int64_t classHandle, int constructorIndex)
    {
        if (!constructor)
        {
            throw std::invalid_argument("registerConstructor: constructor cannot be null");
        }

        std::string key = makeConstructorKey(classHandle, constructorIndex);
        auto it = constructorKeyToHandle.find(key);
        if (it != constructorKeyToHandle.end())
        {
            return it->second;
        }

        int64_t handle = nextHandle++;
        constructorHandles[handle] = ConstructorHandleInfo{constructor, classHandle, constructorIndex};
        constructorKeyToHandle[key] = handle;
        handleTypeMap[handle] = HandleType::CONSTRUCTOR;

        return handle;
    }

    ConstructorHandleInfo ReflectionHandleRegistry::getConstructor(int64_t handle) const
    {
        auto it = constructorHandles.find(handle);
        if (it != constructorHandles.end())
        {
            return it->second;
        }
        return ConstructorHandleInfo{nullptr, -1, -1};
    }
}
