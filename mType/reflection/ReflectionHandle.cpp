#include "ReflectionHandle.hpp"
#include <stdexcept>

namespace reflection
{
    // ========== Cache Key Helper Functions ==========

    std::string ReflectionHandleRegistry::makeFieldKey(int classHandle, const std::string& fieldName)
    {
        return std::to_string(classHandle) + ":" + fieldName;
    }

    std::string ReflectionHandleRegistry::makeMethodKey(int classHandle, const std::string& methodName,
                                                         const std::vector<std::string>& paramTypes)
    {
        std::string key = std::to_string(classHandle) + ":" + methodName + "(";
        for (size_t i = 0; i < paramTypes.size(); ++i)
        {
            if (i > 0) key += ",";
            key += paramTypes[i];
        }
        key += ")";
        return key;
    }

    std::string ReflectionHandleRegistry::makeConstructorKey(int classHandle, int constructorIndex)
    {
        return std::to_string(classHandle) + ":ctor:" + std::to_string(constructorIndex);
    }

    ReflectionHandleRegistry& ReflectionHandleRegistry::instance()
    {
        static ReflectionHandleRegistry instance;
        return instance;
    }

    // ========== Class Handle Management ==========

    int ReflectionHandleRegistry::registerClass(const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef)
    {
        if (!classDef)
        {
            throw std::invalid_argument("registerClass: classDef cannot be null");
        }

        std::lock_guard<std::mutex> lock(mutex);

        // Check if already registered
        const std::string& className = classDef->getName();
        auto it = classNameToHandle.find(className);
        if (it != classNameToHandle.end())
        {
            return it->second;
        }

        // Create new handle
        int handle = nextHandle++;
        classHandles[handle] = classDef;
        classNameToHandle[className] = handle;
        handleTypeMap[handle] = HandleType::CLASS;

        return handle;
    }

    std::shared_ptr<runtimeTypes::klass::ClassDefinition> ReflectionHandleRegistry::getClass(int handle) const
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = classHandles.find(handle);
        if (it != classHandles.end())
        {
            return it->second;
        }
        return nullptr;
    }

    int ReflectionHandleRegistry::findClassHandle(const std::string& className) const
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = classNameToHandle.find(className);
        if (it != classNameToHandle.end())
        {
            return it->second;
        }
        return -1;
    }

    int ReflectionHandleRegistry::getOrCreateClassHandle(const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef)
    {
        if (!classDef)
        {
            throw std::invalid_argument("getOrCreateClassHandle: classDef cannot be null");
        }

        // Try to find existing handle first
        int existingHandle = findClassHandle(classDef->getName());
        if (existingHandle != -1)
        {
            return existingHandle;
        }

        // Register new handle
        return registerClass(classDef);
    }

    // ========== Field Handle Management ==========

    int ReflectionHandleRegistry::registerField(const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field,
                                                int classHandle, const std::string& fieldName)
    {
        if (!field)
        {
            throw std::invalid_argument("registerField: field cannot be null");
        }

        std::lock_guard<std::mutex> lock(mutex);

        // Check cache first - reuse existing handle if available
        std::string key = makeFieldKey(classHandle, fieldName);
        auto it = fieldKeyToHandle.find(key);
        if (it != fieldKeyToHandle.end())
        {
            return it->second;  // Reuse existing handle
        }

        // Create new handle
        int handle = nextHandle++;
        fieldHandles[handle] = FieldHandleInfo{field, classHandle, fieldName};
        fieldKeyToHandle[key] = handle;
        handleTypeMap[handle] = HandleType::FIELD;

        return handle;
    }

    FieldHandleInfo ReflectionHandleRegistry::getField(int handle) const
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = fieldHandles.find(handle);
        if (it != fieldHandles.end())
        {
            return it->second;
        }
        return FieldHandleInfo{nullptr, -1, ""};
    }

    // ========== Method Handle Management ==========

    int ReflectionHandleRegistry::registerMethod(const std::shared_ptr<runtimeTypes::klass::MethodDefinition>& method,
                                                 int classHandle, const std::string& methodName)
    {
        if (!method)
        {
            throw std::invalid_argument("registerMethod: method cannot be null");
        }

        std::lock_guard<std::mutex> lock(mutex);

        // Build parameter type list for signature-based cache key
        std::vector<std::string> paramTypes;
        for (const auto& param : method->getParameters())
        {
            paramTypes.push_back(param.second.toString());
        }

        // Check cache first - reuse existing handle if available
        std::string key = makeMethodKey(classHandle, methodName, paramTypes);
        auto it = methodKeyToHandle.find(key);
        if (it != methodKeyToHandle.end())
        {
            return it->second;  // Reuse existing handle
        }

        // Create new handle
        int handle = nextHandle++;
        methodHandles[handle] = MethodHandleInfo{method, classHandle, methodName};
        methodKeyToHandle[key] = handle;
        handleTypeMap[handle] = HandleType::METHOD;

        return handle;
    }

    MethodHandleInfo ReflectionHandleRegistry::getMethod(int handle) const
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = methodHandles.find(handle);
        if (it != methodHandles.end())
        {
            return it->second;
        }
        return MethodHandleInfo{nullptr, -1, ""};
    }

    // ========== Constructor Handle Management ==========

    int ReflectionHandleRegistry::registerConstructor(const std::shared_ptr<runtimeTypes::klass::ConstructorDefinition>& constructor,
                                                      int classHandle, int constructorIndex)
    {
        if (!constructor)
        {
            throw std::invalid_argument("registerConstructor: constructor cannot be null");
        }

        std::lock_guard<std::mutex> lock(mutex);

        // Check cache first - reuse existing handle if available
        std::string key = makeConstructorKey(classHandle, constructorIndex);
        auto it = constructorKeyToHandle.find(key);
        if (it != constructorKeyToHandle.end())
        {
            return it->second;  // Reuse existing handle
        }

        // Create new handle
        int handle = nextHandle++;
        constructorHandles[handle] = ConstructorHandleInfo{constructor, classHandle, constructorIndex};
        constructorKeyToHandle[key] = handle;
        handleTypeMap[handle] = HandleType::CONSTRUCTOR;

        return handle;
    }

    ConstructorHandleInfo ReflectionHandleRegistry::getConstructor(int handle) const
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = constructorHandles.find(handle);
        if (it != constructorHandles.end())
        {
            return it->second;
        }
        return ConstructorHandleInfo{nullptr, -1, -1};
    }

    // ========== Annotation Handle Management ==========

    int ReflectionHandleRegistry::registerAnnotation(const std::shared_ptr<ast::nodes::annotations::AnnotationNode>& annotation,
                                                     const std::string& annotationName)
    {
        if (!annotation)
        {
            throw std::invalid_argument("registerAnnotation: annotation cannot be null");
        }

        std::lock_guard<std::mutex> lock(mutex);

        // Check cache first using pointer address - reuse existing handle if available
        std::uintptr_t ptrKey = reinterpret_cast<std::uintptr_t>(annotation.get());
        auto it = annotationPtrToHandle.find(ptrKey);
        if (it != annotationPtrToHandle.end())
        {
            return it->second;  // Reuse existing handle
        }

        // Create new handle
        int handle = nextHandle++;
        annotationHandles[handle] = AnnotationHandleInfo{annotation, annotationName};
        annotationPtrToHandle[ptrKey] = handle;
        handleTypeMap[handle] = HandleType::ANNOTATION;

        return handle;
    }

    AnnotationHandleInfo ReflectionHandleRegistry::getAnnotation(int handle) const
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = annotationHandles.find(handle);
        if (it != annotationHandles.end())
        {
            return it->second;
        }
        return AnnotationHandleInfo{nullptr, ""};
    }

    // ========== Utility Methods ==========

    void ReflectionHandleRegistry::releaseHandle(int handle)
    {
        std::lock_guard<std::mutex> lock(mutex);

        // O(1) lookup of handle type
        auto typeIt = handleTypeMap.find(handle);
        if (typeIt == handleTypeMap.end())
        {
            return;  // Handle not found
        }

        HandleType type = typeIt->second;
        handleTypeMap.erase(typeIt);

        switch (type)
        {
            case HandleType::CLASS:
            {
                auto classIt = classHandles.find(handle);
                if (classIt != classHandles.end())
                {
                    if (classIt->second)
                    {
                        classNameToHandle.erase(classIt->second->getName());
                    }
                    classHandles.erase(classIt);
                }
                break;
            }
            case HandleType::FIELD:
            {
                auto fieldIt = fieldHandles.find(handle);
                if (fieldIt != fieldHandles.end())
                {
                    std::string key = makeFieldKey(fieldIt->second.classHandle, fieldIt->second.fieldName);
                    fieldKeyToHandle.erase(key);
                    fieldHandles.erase(fieldIt);
                }
                break;
            }
            case HandleType::METHOD:
            {
                auto methodIt = methodHandles.find(handle);
                if (methodIt != methodHandles.end())
                {
                    if (methodIt->second.method)
                    {
                        std::vector<std::string> paramTypes;
                        for (const auto& param : methodIt->second.method->getParameters())
                        {
                            paramTypes.push_back(param.second.toString());
                        }
                        std::string key = makeMethodKey(methodIt->second.classHandle, methodIt->second.methodName, paramTypes);
                        methodKeyToHandle.erase(key);
                    }
                    methodHandles.erase(methodIt);
                }
                break;
            }
            case HandleType::CONSTRUCTOR:
            {
                auto ctorIt = constructorHandles.find(handle);
                if (ctorIt != constructorHandles.end())
                {
                    std::string key = makeConstructorKey(ctorIt->second.classHandle, ctorIt->second.constructorIndex);
                    constructorKeyToHandle.erase(key);
                    constructorHandles.erase(ctorIt);
                }
                break;
            }
            case HandleType::ANNOTATION:
            {
                auto annoIt = annotationHandles.find(handle);
                if (annoIt != annotationHandles.end())
                {
                    if (annoIt->second.annotation)
                    {
                        std::uintptr_t ptrKey = reinterpret_cast<std::uintptr_t>(annoIt->second.annotation.get());
                        annotationPtrToHandle.erase(ptrKey);
                    }
                    annotationHandles.erase(annoIt);
                }
                break;
            }
        }
    }

    void ReflectionHandleRegistry::clear()
    {
        std::lock_guard<std::mutex> lock(mutex);

        // Clear primary handle maps
        classHandles.clear();
        fieldHandles.clear();
        methodHandles.clear();
        constructorHandles.clear();
        annotationHandles.clear();

        // Clear all reverse mapping caches
        classNameToHandle.clear();
        fieldKeyToHandle.clear();
        methodKeyToHandle.clear();
        constructorKeyToHandle.clear();
        annotationPtrToHandle.clear();

        // Clear handle type tracking
        handleTypeMap.clear();

        nextHandle = 1;
    }

    size_t ReflectionHandleRegistry::getTotalHandleCount() const
    {
        std::lock_guard<std::mutex> lock(mutex);

        return classHandles.size() + fieldHandles.size() + methodHandles.size() +
               constructorHandles.size() + annotationHandles.size();
    }

} // namespace reflection
