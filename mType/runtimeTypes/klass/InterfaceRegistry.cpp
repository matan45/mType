#include "InterfaceRegistry.hpp"
#include "../../circularDependency/TrueCyclicException.hpp"
#include "../../circularDependency/DepthLimitException.hpp"
#include <unordered_set>
#include <chrono>

namespace runtimeTypes::klass
{
    InterfaceRegistry::InterfaceRegistry()
    {
        // Initialize enhanced circular dependency detection for interface inheritance
        circularDependency::CircularDependencyConfig config;
        config.maxInterfaceDepth = 20; // Reasonable depth for interface inheritance
        config.enableEarlyDetection = true;
        config.enablePerformanceMetrics = true;

        dependencyDetector = std::make_shared<circularDependency::CircularDependencyDetector>(config);
    }

    void InterfaceRegistry::registerInterface(const std::string& name, std::shared_ptr<InterfaceDefinition> interface)
    {
        // Check if we need to evict before adding
        if (shouldEvictInterfaces())
        {
            performAutomaticCleanup();
        }

        interfaces[name] = interface;
        updateInterfaceAccess(name);

        // Clear validation cache since interface hierarchy may have changed
        clearValidationCache();
    }

    std::shared_ptr<InterfaceDefinition> InterfaceRegistry::findInterface(const std::string& name) const
    {
        // First try direct lookup (for non-generic interfaces)
        auto it = interfaces.find(name);
        if (it != interfaces.end())
        {
            updateInterfaceAccess(name);
            return it->second;
        }

        // If not found and name contains generic parameters, try base name lookup
        if (name.find('<') != std::string::npos)
        {
            std::string baseName = extractBaseTypeName(name);
            auto baseIt = interfaces.find(baseName);
            if (baseIt != interfaces.end())
            {
                updateInterfaceAccess(baseName);
                return baseIt->second;
            }
        }

        return nullptr;
    }

    bool InterfaceRegistry::hasInterface(const std::string& name) const
    {
        // First try direct lookup
        if (interfaces.find(name) != interfaces.end())
        {
            return true;
        }

        // If not found and name contains generic parameters, try base name lookup
        if (name.find('<') != std::string::npos)
        {
            std::string baseName = extractBaseTypeName(name);
            return interfaces.find(baseName) != interfaces.end();
        }

        return false;
    }

    const std::unordered_map<std::string, std::shared_ptr<InterfaceDefinition>>& InterfaceRegistry::
    getAllInterfaces() const
    {
        return interfaces;
    }

    void InterfaceRegistry::clear()
    {
        interfaces.clear();
        interfaceAccessTimes.clear();
        interfaceAccessOrder.clear();
        interfaceOrderMap.clear();
        clearValidationCache();
    }

    bool InterfaceRegistry::removeInterface(const std::string& name)
    {
        auto it = interfaces.find(name);
        if (it != interfaces.end())
        {
            interfaces.erase(it);

            // Clean up interface LRU tracking
            interfaceAccessTimes.erase(name);
            auto orderIt = interfaceOrderMap.find(name);
            if (orderIt != interfaceOrderMap.end())
            {
                interfaceAccessOrder.erase(orderIt->second);
                interfaceOrderMap.erase(orderIt);
            }

            // Clear validation cache since interface structure changed
            clearValidationCache();
            return true;
        }
        return false;
    }

    size_t InterfaceRegistry::count() const
    {
        return interfaces.size();
    }

    std::vector<std::string> InterfaceRegistry::findUnusedInterfaces() const
    {
        std::vector<std::string> unused;
        for (const auto& [name, interfaceDef] : interfaces)
        {
            // Simple heuristic: if use_count is 1, only registry holds it
            if (interfaceDef.use_count() == 1)
            {
                unused.push_back(name);
            }
        }
        return unused;
    }

    size_t InterfaceRegistry::cleanupUnusedInterfaces()
    {
        return removeInterfacesIf([](const std::string&, const std::shared_ptr<InterfaceDefinition>& def)
        {
            return def.use_count() == 1; // Only registry references it
        });
    }

    bool InterfaceRegistry::validateInterfaceHierarchy(const std::string& interfaceName) const
    {
        // Check cache first - O(1) lookup for previously validated interfaces
        if (validatedInterfaces.find(interfaceName) != validatedInterfaces.end())
        {
            updateValidationAccess(interfaceName);
            return true; // Already validated successfully
        }

        // Perform validation only if not cached
        try
        {
            validateInterfaceHierarchyEnhanced(interfaceName);

            // Check if we need to evict before adding to cache
            if (validatedInterfaces.size() >= MAX_VALIDATION_CACHE)
            {
                evictLRUValidations();
            }

            // Cache successful validation result with LRU tracking
            validatedInterfaces.insert(interfaceName);
            updateValidationAccess(interfaceName);
            return true;
        }
        catch (const circularDependency::TrueCyclicException&)
        {
            return false; // True circular dependency - don't cache failed validations
        }
        catch (const circularDependency::DepthLimitException&)
        {
            return false; // Depth limit exceeded - don't cache failed validations
        }
    }

    void InterfaceRegistry::setInterfaceDependencyConfig(const circularDependency::CircularDependencyConfig& config)
    {
        dependencyDetector->setConfig(config);
        // Clear cache when configuration changes as limits may affect validation
        validatedInterfaces.clear();
    }

    circularDependency::CircularDependencyConfig InterfaceRegistry::getInterfaceDependencyConfig() const
    {
        return dependencyDetector->getConfig();
    }

    void InterfaceRegistry::clearValidationCache()
    {
        validatedInterfaces.clear();
        validationAccessOrder.clear();
        validationOrderMap.clear();
    }

    size_t InterfaceRegistry::getValidationCacheSize() const
    {
        return validatedInterfaces.size();
    }

    void InterfaceRegistry::validateInterfaceHierarchyEnhanced(const std::string& interfaceName) const
    {
        // Use RAII helper for automatic cleanup
        circularDependency::DependencyScope scope(
            *dependencyDetector,
            circularDependency::DependencyType::INTERFACE_INHERITANCE,
            interfaceName,
            "interface hierarchy validation"
        );

        auto interface = findInterface(interfaceName);
        if (!interface)
        {
            return; // Interface not found, but that's okay for validation
        }

        // Recursively validate all extended interfaces
        for (const auto& extendedInterface : interface->getExtendedInterfaces())
        {
            validateInterfaceHierarchyEnhanced(extendedInterface);
        }
    }

    void InterfaceRegistry::updateInterfaceAccess(const std::string& name) const
    {
        // Update access time
        interfaceAccessTimes[name] = std::chrono::steady_clock::now();

        // Move to front of LRU list (most recently used)
        auto orderIt = interfaceOrderMap.find(name);
        if (orderIt != interfaceOrderMap.end())
        {
            interfaceAccessOrder.erase(orderIt->second);
        }
        interfaceAccessOrder.push_front(name);
        interfaceOrderMap[name] = interfaceAccessOrder.begin();
    }

    void InterfaceRegistry::evictLRUInterfaces() const
    {
        // Evict least recently used interfaces until we're under the cleanup threshold
        while (interfaces.size() > CLEANUP_THRESHOLD && !interfaceAccessOrder.empty())
        {
            const std::string& lruInterface = interfaceAccessOrder.back();

            // Only evict if use_count is 1 (only registry holds reference)
            auto it = interfaces.find(lruInterface);
            if (it != interfaces.end() && it->second.use_count() == 1)
            {
                interfaces.erase(it);
                interfaceAccessTimes.erase(lruInterface);
                interfaceOrderMap.erase(lruInterface);
                interfaceAccessOrder.pop_back();
            }
            else
            {
                // Can't evict this one, move to front and try next
                interfaceAccessOrder.pop_back();
                interfaceAccessOrder.push_front(lruInterface);
                interfaceOrderMap[lruInterface] = interfaceAccessOrder.begin();
                break; // Avoid infinite loop if all interfaces are in use
            }
        }
    }

    void InterfaceRegistry::updateValidationAccess(const std::string& name) const
    {
        // Move to front of validation cache LRU list
        auto orderIt = validationOrderMap.find(name);
        if (orderIt != validationOrderMap.end())
        {
            validationAccessOrder.erase(orderIt->second);
        }
        validationAccessOrder.push_front(name);
        validationOrderMap[name] = validationAccessOrder.begin();
    }

    void InterfaceRegistry::evictLRUValidations() const
    {
        // Evict validation cache entries until we're under the limit
        while (validatedInterfaces.size() > static_cast<size_t>(MAX_VALIDATION_CACHE * 0.8) &&
            !validationAccessOrder.empty())
        {
            const std::string& lruValidation = validationAccessOrder.back();
            validatedInterfaces.erase(lruValidation);
            validationOrderMap.erase(lruValidation);
            validationAccessOrder.pop_back();
        }
    }

    bool InterfaceRegistry::shouldEvictInterfaces() const
    {
        return interfaces.size() >= MAX_INTERFACES ||
        (interfaces.size() >= CLEANUP_THRESHOLD &&
            interfaceAccessOrder.size() >= EVICTION_BATCH_SIZE);
    }

    void InterfaceRegistry::performAutomaticCleanup() const
    {
        // If we still need space, evict LRU interfaces
        if (interfaces.size() >= CLEANUP_THRESHOLD)
        {
            evictLRUInterfaces();
        }

        // Also clean up validation cache if needed
        if (validatedInterfaces.size() >= MAX_VALIDATION_CACHE)
        {
            evictLRUValidations();
        }
    }

    std::string InterfaceRegistry::extractBaseTypeName(const std::string& typeName) const
    {
        // Extract base type name from generic instantiation (e.g., "Predicate<Person>" -> "Predicate")
        size_t anglePos = typeName.find('<');
        if (anglePos != std::string::npos)
        {
            return typeName.substr(0, anglePos);
        }
        return typeName;
    }
}
