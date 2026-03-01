#include "ReifiedTypeRegistry.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"

namespace types
{
    UnifiedTypePtr ReifiedTypeRegistry::intern(const UnifiedTypePtr& type)
    {
        if (!type)
        {
            return nullptr;
        }

        std::string canonicalStr = type->toCanonicalString();

        std::lock_guard<std::mutex> lock(typeCacheMutex);

        auto it = typeCache.find(canonicalStr);
        if (it != typeCache.end())
        {
            return it->second;
        }

        // Store and return the provided type
        typeCache[canonicalStr] = type;
        return type;
    }

    bool ReifiedTypeRegistry::isInterned(const UnifiedTypePtr& type) const
    {
        if (!type)
        {
            return false;
        }

        std::string canonicalStr = type->toCanonicalString();

        std::lock_guard<std::mutex> lock(typeCacheMutex);
        return typeCache.find(canonicalStr) != typeCache.end();
    }

    UnifiedTypePtr ReifiedTypeRegistry::findByCanonicalString(
        const std::string& canonicalString) const
    {
        std::lock_guard<std::mutex> lock(typeCacheMutex);

        auto it = typeCache.find(canonicalString);
        if (it != typeCache.end())
        {
            return it->second;
        }

        return nullptr;
    }

    bool ReifiedTypeRegistry::isInstance(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj,
        const UnifiedTypePtr& type) const
    {
        if (!obj || !type)
        {
            return false;
        }

        // Check the object's own reified type
        UnifiedTypePtr objectType = getReifiedType(obj);
        if (objectType && isTypeCompatible(objectType, type))
        {
            return true;
        }

        // Walk the inheritance chain: check if any ancestor's reified type matches
        // This handles cases like StringBox extends Box<String> where
        // isInstance(stringBoxObj, Box<String>) should return true
        auto classDef = obj->getClassDefinition();
        if (classDef)
        {
            auto parentDef = classDef->getParentClass();
            while (parentDef)
            {
                UnifiedTypePtr parentReified = parentDef->getReifiedType();
                if (parentReified && isTypeCompatible(parentReified, type))
                {
                    return true;
                }
                parentDef = parentDef->getParentClass();
            }
        }

        return false;
    }

    UnifiedTypePtr ReifiedTypeRegistry::getReifiedType(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj) const
    {
        if (!obj)
        {
            return nullptr;
        }

        const void* key = obj.get();

        std::lock_guard<std::mutex> lock(objectRegistryMutex);

        auto it = objectTypes.find(key);
        if (it != objectTypes.end())
        {
            // Verify the weak_ptr is still valid
            if (!it->second.first.expired())
            {
                return it->second.second;
            }
        }

        return nullptr;
    }

    void ReifiedTypeRegistry::registerObjectType(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj,
        const UnifiedTypePtr& type)
    {
        if (!obj || !type)
        {
            return;
        }

        // Intern the type first to ensure canonical representation
        UnifiedTypePtr internedType = intern(type);

        const void* key = obj.get();

        {
            std::lock_guard<std::mutex> lock(objectRegistryMutex);
            objectTypes[key] = std::make_pair(
                std::weak_ptr<runtimeTypes::klass::ObjectInstance>(obj),
                internedType);
            registrationsSinceCleanup++;
        }

        // Periodically clean up orphaned registrations to prevent unbounded growth
        if (registrationsSinceCleanup >= CLEANUP_THRESHOLD)
        {
            registrationsSinceCleanup = 0;
            cleanupOrphanedRegistrations();
        }
    }

    void ReifiedTypeRegistry::unregisterObject(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj)
    {
        if (!obj)
        {
            return;
        }

        const void* key = obj.get();

        std::lock_guard<std::mutex> lock(objectRegistryMutex);
        objectTypes.erase(key);
    }

    std::vector<UnifiedTypePtr> ReifiedTypeRegistry::getTypeArguments(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj) const
    {
        UnifiedTypePtr reifiedType = getReifiedType(obj);
        if (reifiedType)
        {
            return std::vector<UnifiedTypePtr>(
                reifiedType->getTypeArguments().begin(),
                reifiedType->getTypeArguments().end());
        }

        return {};
    }

    bool ReifiedTypeRegistry::haveSameReifiedType(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj1,
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj2) const
    {
        UnifiedTypePtr type1 = getReifiedType(obj1);
        UnifiedTypePtr type2 = getReifiedType(obj2);

        if (!type1 && !type2)
        {
            return true;  // Both unregistered
        }

        if (!type1 || !type2)
        {
            return false;  // One registered, one not
        }

        return type1->equals(type2);
    }

    std::vector<UnifiedTypePtr> ReifiedTypeRegistry::getAllInternedTypes() const
    {
        std::lock_guard<std::mutex> lock(typeCacheMutex);

        std::vector<UnifiedTypePtr> result;
        result.reserve(typeCache.size());

        for (const auto& [key, type] : typeCache)
        {
            result.push_back(type);
        }

        return result;
    }

    size_t ReifiedTypeRegistry::getInternedTypeCount() const
    {
        std::lock_guard<std::mutex> lock(typeCacheMutex);
        return typeCache.size();
    }

    size_t ReifiedTypeRegistry::getRegisteredObjectCount() const
    {
        std::lock_guard<std::mutex> lock(objectRegistryMutex);
        return objectTypes.size();
    }

    void ReifiedTypeRegistry::clear()
    {
        {
            std::lock_guard<std::mutex> lock(typeCacheMutex);
            typeCache.clear();
        }

        {
            std::lock_guard<std::mutex> lock(objectRegistryMutex);
            objectTypes.clear();
        }
    }

    size_t ReifiedTypeRegistry::cleanupOrphanedRegistrations()
    {
        std::lock_guard<std::mutex> lock(objectRegistryMutex);

        std::vector<const void*> toRemove;

        for (const auto& [key, pair] : objectTypes)
        {
            if (pair.first.expired())
            {
                toRemove.push_back(key);
            }
        }

        for (const void* key : toRemove)
        {
            objectTypes.erase(key);
        }

        return toRemove.size();
    }

    bool ReifiedTypeRegistry::isTypeCompatible(
        const UnifiedTypePtr& objectType,
        const UnifiedTypePtr& expectedType) const
    {
        if (!objectType || !expectedType)
        {
            return false;
        }

        // Exact match
        if (objectType->equals(expectedType))
        {
            return true;
        }

        // If expected type has no type arguments, only compare base types
        if (!expectedType->isParameterized())
        {
            // Compare base type names
            return objectType->getName() == expectedType->getName();
        }

        // Both are parameterized - must match exactly including type arguments
        if (objectType->getName() != expectedType->getName())
        {
            return false;
        }

        const auto& objArgs = objectType->getTypeArguments();
        const auto& expArgs = expectedType->getTypeArguments();

        if (objArgs.size() != expArgs.size())
        {
            return false;
        }

        // All type arguments must match
        for (size_t i = 0; i < objArgs.size(); ++i)
        {
            if (!objArgs[i] || !expArgs[i])
            {
                return false;
            }

            if (!objArgs[i]->equals(expArgs[i]))
            {
                return false;
            }
        }

        return true;
    }
}
