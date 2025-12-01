#include "ReflectionHandle.hpp"

namespace reflection
{
    ReflectionHandleRegistry& ReflectionHandleRegistry::instance()
    {
        static ReflectionHandleRegistry instance;
        return instance;
    }

    // ========== Class Handle Management ==========

    int ReflectionHandleRegistry::registerClass(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef)
    {
        if (!classDef)
        {
            return -1;
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

    int ReflectionHandleRegistry::getOrCreateClassHandle(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef)
    {
        if (!classDef)
        {
            return -1;
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

    int ReflectionHandleRegistry::registerField(std::shared_ptr<runtimeTypes::klass::FieldDefinition> field,
                                                int classHandle, const std::string& fieldName)
    {
        if (!field)
        {
            return -1;
        }

        std::lock_guard<std::mutex> lock(mutex);

        int handle = nextHandle++;
        fieldHandles[handle] = FieldHandleInfo{field, classHandle, fieldName};

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

    int ReflectionHandleRegistry::registerMethod(std::shared_ptr<runtimeTypes::klass::MethodDefinition> method,
                                                 int classHandle, const std::string& methodName)
    {
        if (!method)
        {
            return -1;
        }

        std::lock_guard<std::mutex> lock(mutex);

        int handle = nextHandle++;
        methodHandles[handle] = MethodHandleInfo{method, classHandle, methodName};

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

    int ReflectionHandleRegistry::registerConstructor(std::shared_ptr<runtimeTypes::klass::ConstructorDefinition> constructor,
                                                      int classHandle, int constructorIndex)
    {
        if (!constructor)
        {
            return -1;
        }

        std::lock_guard<std::mutex> lock(mutex);

        int handle = nextHandle++;
        constructorHandles[handle] = ConstructorHandleInfo{constructor, classHandle, constructorIndex};

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

    int ReflectionHandleRegistry::registerAnnotation(std::shared_ptr<ast::nodes::annotations::AnnotationNode> annotation,
                                                     const std::string& annotationName)
    {
        if (!annotation)
        {
            return -1;
        }

        std::lock_guard<std::mutex> lock(mutex);

        int handle = nextHandle++;
        annotationHandles[handle] = AnnotationHandleInfo{annotation, annotationName};

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

        // Try to remove from each map
        auto classIt = classHandles.find(handle);
        if (classIt != classHandles.end())
        {
            // Also remove from reverse mapping
            if (classIt->second)
            {
                classNameToHandle.erase(classIt->second->getName());
            }
            classHandles.erase(classIt);
            return;
        }

        fieldHandles.erase(handle);
        methodHandles.erase(handle);
        constructorHandles.erase(handle);
        annotationHandles.erase(handle);
    }

    void ReflectionHandleRegistry::clear()
    {
        std::lock_guard<std::mutex> lock(mutex);

        classHandles.clear();
        classNameToHandle.clear();
        fieldHandles.clear();
        methodHandles.clear();
        constructorHandles.clear();
        annotationHandles.clear();

        nextHandle = 1;
    }

    size_t ReflectionHandleRegistry::getTotalHandleCount() const
    {
        std::lock_guard<std::mutex> lock(mutex);

        return classHandles.size() + fieldHandles.size() + methodHandles.size() +
               constructorHandles.size() + annotationHandles.size();
    }

} // namespace reflection
