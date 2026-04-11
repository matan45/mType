#include "ReflectionHandle.hpp"
#include "../types/ReifiedTypeRegistry.hpp"
#include <stdexcept>

namespace reflection
{
    // ========== Cache Key Helper Functions ==========

    std::string ReflectionHandleRegistry::makeFieldKey(int64_t classHandle, const std::string& fieldName)
    {
        return std::to_string(classHandle) + ":" + fieldName;
    }

    std::string ReflectionHandleRegistry::makeMethodKey(int64_t classHandle, const std::string& methodName,
                                                         const std::vector<std::string>& paramTypes)
    {
        // Use length-prefixed encoding to avoid delimiter collisions with generic types like Foo<A,B>
        // Format: "classHandle:methodName#count:len1:type1:len2:type2:..."
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

    // ========== Class Handle Management ==========

    int64_t ReflectionHandleRegistry::registerClass(const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef)
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
        int64_t handle = nextHandle++;
        classHandles[handle] = classDef;
        classNameToHandle[className] = handle;
        handleTypeMap[handle] = HandleType::CLASS;

        return handle;
    }

    std::shared_ptr<runtimeTypes::klass::ClassDefinition> ReflectionHandleRegistry::getClass(int64_t handle) const
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = classHandles.find(handle);
        if (it != classHandles.end())
        {
            return it->second;
        }
        return nullptr;
    }

    int64_t ReflectionHandleRegistry::findClassHandle(const std::string& className) const
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = classNameToHandle.find(className);
        if (it != classNameToHandle.end())
        {
            return it->second;
        }
        return -1;
    }

    int64_t ReflectionHandleRegistry::getOrCreateClassHandle(const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef)
    {
        if (!classDef)
        {
            throw std::invalid_argument("getOrCreateClassHandle: classDef cannot be null");
        }

        // Try to find existing handle first
        int64_t existingHandle = findClassHandle(classDef->getName());
        if (existingHandle != -1)
        {
            return existingHandle;
        }

        // Register new handle
        return registerClass(classDef);
    }

    int64_t ReflectionHandleRegistry::getOrCreateClosedClassHandle(
        const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& baseDef,
        const ::types::UnifiedTypePtr& reified)
    {
        if (!baseDef)
        {
            throw std::invalid_argument("getOrCreateClosedClassHandle: baseDef cannot be null");
        }
        if (!reified)
        {
            throw std::invalid_argument("getOrCreateClosedClassHandle: reified cannot be null");
        }

        const std::string canonical = reified->toCanonicalString();

        std::lock_guard<std::mutex> lock(mutex);

        // Dedup: reuse the closed handle for this canonical form if one exists.
        auto it = canonicalStringToHandle.find(canonical);
        if (it != canonicalStringToHandle.end())
        {
            return it->second;
        }

        // Mint a fresh handle. Closed handles still point at the base
        // ClassDefinition in classHandles so every existing native that does
        // `registry.getClass(handle)` keeps working on a closed handle.
        //
        // NOTE: we deliberately do NOT touch classNameToHandle here — that map
        // stays keyed on raw names, which is what keeps open and closed handles
        // distinct (forName("Box") vs forName("Box<Int>")).
        int64_t handle = nextHandle++;
        classHandles[handle] = baseDef;
        closedHandleReified[handle] = reified;
        canonicalStringToHandle[canonical] = handle;
        handleTypeMap[handle] = HandleType::CLASS;

        return handle;
    }

    ::types::UnifiedTypePtr ReflectionHandleRegistry::getReifiedType(int64_t handle) const
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = closedHandleReified.find(handle);
        if (it != closedHandleReified.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool ReflectionHandleRegistry::isClosedHandle(int64_t handle) const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return closedHandleReified.find(handle) != closedHandleReified.end();
    }

    // ========== Field Handle Management ==========

    int64_t ReflectionHandleRegistry::registerField(const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field,
                                                int64_t classHandle, const std::string& fieldName)
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
        int64_t handle = nextHandle++;
        fieldHandles[handle] = FieldHandleInfo{field, classHandle, fieldName};
        fieldKeyToHandle[key] = handle;
        handleTypeMap[handle] = HandleType::FIELD;

        return handle;
    }

    FieldHandleInfo ReflectionHandleRegistry::getField(int64_t handle) const
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

    int64_t ReflectionHandleRegistry::registerMethod(const std::shared_ptr<runtimeTypes::klass::MethodDefinition>& method,
                                                 int64_t classHandle, const std::string& methodName)
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
        int64_t handle = nextHandle++;
        methodHandles[handle] = MethodHandleInfo{method, classHandle, methodName};
        methodKeyToHandle[key] = handle;
        handleTypeMap[handle] = HandleType::METHOD;

        return handle;
    }

    MethodHandleInfo ReflectionHandleRegistry::getMethod(int64_t handle) const
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

    int64_t ReflectionHandleRegistry::registerConstructor(const std::shared_ptr<runtimeTypes::klass::ConstructorDefinition>& constructor,
                                                      int64_t classHandle, int constructorIndex)
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
        int64_t handle = nextHandle++;
        constructorHandles[handle] = ConstructorHandleInfo{constructor, classHandle, constructorIndex};
        constructorKeyToHandle[key] = handle;
        handleTypeMap[handle] = HandleType::CONSTRUCTOR;

        return handle;
    }

    ConstructorHandleInfo ReflectionHandleRegistry::getConstructor(int64_t handle) const
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

    int64_t ReflectionHandleRegistry::registerAnnotation(const std::shared_ptr<ast::nodes::annotations::AnnotationNode>& annotation,
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
        int64_t handle = nextHandle++;
        annotationHandles[handle] = AnnotationHandleInfo{annotation, annotationName};
        annotationPtrToHandle[ptrKey] = handle;
        handleTypeMap[handle] = HandleType::ANNOTATION;

        return handle;
    }

    AnnotationHandleInfo ReflectionHandleRegistry::getAnnotation(int64_t handle) const
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

    void ReflectionHandleRegistry::releaseHandle(int64_t handle)
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
                // Erase closed-form entries first so we don't leave orphan
                // side-map state if this handle was a parameterized closed form.
                auto closedIt = closedHandleReified.find(handle);
                if (closedIt != closedHandleReified.end())
                {
                    if (closedIt->second)
                    {
                        canonicalStringToHandle.erase(closedIt->second->toCanonicalString());
                    }
                    closedHandleReified.erase(closedIt);
                }

                auto classIt = classHandles.find(handle);
                if (classIt != classHandles.end())
                {
                    // Only erase from classNameToHandle if THIS handle is the
                    // one indexed by the class's raw name. For closed handles
                    // classNameToHandle either has no entry or points at the
                    // open-form handle — don't clobber that.
                    if (classIt->second)
                    {
                        const std::string& rawName = classIt->second->getName();
                        auto nameIt = classNameToHandle.find(rawName);
                        if (nameIt != classNameToHandle.end() && nameIt->second == handle)
                        {
                            classNameToHandle.erase(nameIt);
                        }
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

        // Clear closed-form parameterized class handle maps
        closedHandleReified.clear();
        canonicalStringToHandle.clear();

        nextHandle = 1;
    }

    size_t ReflectionHandleRegistry::getTotalHandleCount() const
    {
        std::lock_guard<std::mutex> lock(mutex);

        return classHandles.size() + fieldHandles.size() + methodHandles.size() +
               constructorHandles.size() + annotationHandles.size();
    }

} // namespace reflection
