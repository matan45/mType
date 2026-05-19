#include "ReflectionHandle.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include "../types/ReifiedTypeRegistry.hpp"

namespace reflection
{
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

        auto it = canonicalStringToHandle.find(canonical);
        if (it != canonicalStringToHandle.end())
        {
            return it->second;
        }

        // Closed handles still point at the base ClassDefinition in classHandles
        // so every existing native that does `registry.getClass(handle)` keeps
        // working on a closed handle.
        // Deliberately do NOT touch classNameToHandle here — that map stays
        // keyed on raw names, which keeps open and closed handles distinct
        // (forName("Box") vs forName("Box<Int>")).
        int64_t handle = nextHandle++;
        classHandles[handle] = baseDef;
        closedHandleReified[handle] = reified;
        canonicalStringToHandle[canonical] = handle;
        handleTypeMap[handle] = HandleType::CLASS;

        return handle;
    }

    ::types::UnifiedTypePtr ReflectionHandleRegistry::getReifiedType(int64_t handle) const
    {
        auto it = closedHandleReified.find(handle);
        if (it != closedHandleReified.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool ReflectionHandleRegistry::isClosedHandle(int64_t handle) const
    {
        return closedHandleReified.find(handle) != closedHandleReified.end();
    }

    const FieldHandleInfo* ReflectionHandleRegistry::getFieldInfo(int64_t handle) const
    {
        auto it = fieldHandles.find(handle);
        return it != fieldHandles.end() ? &it->second : nullptr;
    }

    int64_t ReflectionHandleRegistry::findCachedFieldLookup(
        int64_t classHandle,
        const std::string& fieldName,
        bool declaredOnly) const
    {
        auto it = fieldLookupKeyToHandle.find(FieldLookupKey{classHandle, declaredOnly, fieldName});
        return it != fieldLookupKeyToHandle.end() ? it->second : -1;
    }

    void ReflectionHandleRegistry::cacheFieldLookup(
        int64_t classHandle,
        const std::string& fieldName,
        bool declaredOnly,
        int64_t fieldHandle)
    {
        fieldLookupKeyToHandle[FieldLookupKey{classHandle, declaredOnly, fieldName}] = fieldHandle;
    }

    const MethodHandleInfo* ReflectionHandleRegistry::getMethodInfo(int64_t handle) const
    {
        auto it = methodHandles.find(handle);
        return it != methodHandles.end() ? &it->second : nullptr;
    }

    int64_t ReflectionHandleRegistry::findCachedMethodLookup(
        int64_t classHandle,
        const std::string& methodName,
        size_t paramCount,
        bool declaredOnly) const
    {
        auto it = methodLookupKeyToHandle.find(MethodLookupKey{
            classHandle, paramCount, declaredOnly, methodName});
        return it != methodLookupKeyToHandle.end() ? it->second : -1;
    }

    void ReflectionHandleRegistry::cacheMethodLookup(
        int64_t classHandle,
        const std::string& methodName,
        size_t paramCount,
        bool declaredOnly,
        int64_t methodHandle)
    {
        methodLookupKeyToHandle[MethodLookupKey{
            classHandle, paramCount, declaredOnly, methodName}] = methodHandle;
    }

    int64_t ReflectionHandleRegistry::findCachedConstructorLookup(
        int64_t classHandle,
        size_t paramCount,
        bool declaredOnly) const
    {
        auto it = constructorLookupKeyToHandle.find(ConstructorLookupKey{
            classHandle, paramCount, declaredOnly});
        return it != constructorLookupKeyToHandle.end() ? it->second : -1;
    }

    void ReflectionHandleRegistry::cacheConstructorLookup(
        int64_t classHandle,
        size_t paramCount,
        bool declaredOnly,
        int64_t constructorHandle)
    {
        constructorLookupKeyToHandle[ConstructorLookupKey{
            classHandle, paramCount, declaredOnly}] = constructorHandle;
    }

    const ConstructorHandleInfo* ReflectionHandleRegistry::getConstructorInfo(int64_t handle) const
    {
        auto it = constructorHandles.find(handle);
        return it != constructorHandles.end() ? &it->second : nullptr;
    }

    int64_t ReflectionHandleRegistry::registerAnnotation(
        const std::shared_ptr<ast::nodes::annotations::AnnotationNode>& annotation,
        const std::string& annotationName)
    {
        if (!annotation)
        {
            throw std::invalid_argument("registerAnnotation: annotation cannot be null");
        }

        std::uintptr_t ptrKey = reinterpret_cast<std::uintptr_t>(annotation.get());
        auto it = annotationPtrToHandle.find(ptrKey);
        if (it != annotationPtrToHandle.end())
        {
            return it->second;
        }

        int64_t handle = nextHandle++;
        annotationHandles[handle] = AnnotationHandleInfo{annotation, annotationName};
        annotationPtrToHandle[ptrKey] = handle;
        handleTypeMap[handle] = HandleType::ANNOTATION;

        return handle;
    }

    AnnotationHandleInfo ReflectionHandleRegistry::getAnnotation(int64_t handle) const
    {
        auto it = annotationHandles.find(handle);
        if (it != annotationHandles.end())
        {
            return it->second;
        }
        return AnnotationHandleInfo{nullptr, ""};
    }

    void ReflectionHandleRegistry::releaseHandle(int64_t handle)
    {
        auto typeIt = handleTypeMap.find(handle);
        if (typeIt == handleTypeMap.end())
        {
            return;
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
                    fieldLookupKeyToHandle.erase(FieldLookupKey{
                        fieldIt->second.classHandle,
                        true,
                        fieldIt->second.fieldName});
                    fieldLookupKeyToHandle.erase(FieldLookupKey{
                        fieldIt->second.classHandle,
                        false,
                        fieldIt->second.fieldName});
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
                        std::string key = makeMethodKey(methodIt->second.classHandle, methodIt->second.methodName,
                                                        paramTypes);
                        methodKeyToHandle.erase(key);
                        methodLookupKeyToHandle.erase(MethodLookupKey{
                            methodIt->second.classHandle,
                            methodIt->second.userParamCount,
                            true,
                            methodIt->second.methodName});
                        methodLookupKeyToHandle.erase(MethodLookupKey{
                            methodIt->second.classHandle,
                            methodIt->second.userParamCount,
                            false,
                            methodIt->second.methodName});
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
                    if (ctorIt->second.constructor)
                    {
                        constructorLookupKeyToHandle.erase(ConstructorLookupKey{
                            ctorIt->second.classHandle,
                            ctorIt->second.constructor->getParameterCount(),
                            true});
                        constructorLookupKeyToHandle.erase(ConstructorLookupKey{
                            ctorIt->second.classHandle,
                            ctorIt->second.constructor->getParameterCount(),
                            false});
                    }
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
        classHandles.clear();
        fieldHandles.clear();
        methodHandles.clear();
        constructorHandles.clear();
        annotationHandles.clear();

        classNameToHandle.clear();
        fieldKeyToHandle.clear();
        methodKeyToHandle.clear();
        fieldLookupKeyToHandle.clear();
        methodLookupKeyToHandle.clear();
        constructorLookupKeyToHandle.clear();
        constructorKeyToHandle.clear();
        annotationPtrToHandle.clear();

        handleTypeMap.clear();

        closedHandleReified.clear();
        canonicalStringToHandle.clear();

        nextHandle = 1;
    }

    size_t ReflectionHandleRegistry::getTotalHandleCount() const
    {
        return classHandles.size() + fieldHandles.size() + methodHandles.size() +
            constructorHandles.size() + annotationHandles.size();
    }
}
