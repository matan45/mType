#pragma once

#include "InterfaceDefinition.hpp"
#include <chrono>
#include <cstddef>
#include "../../circularDependency/CircularDependencyDetector.hpp"
#include "../../circularDependency/CircularDependencyException.hpp"
#include "../../circularDependency/DepthLimitException.hpp"
#include "../../circularDependency/DependencyScope.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace circularDependency
{
    class TrueCyclicException;
}

namespace runtimeTypes::klass
{
    class InterfaceRegistry
    {
    private:
        // Configuration for memory management
        static constexpr size_t MAX_INTERFACES = 10000; // Maximum stored interfaces
        static constexpr size_t MAX_VALIDATION_CACHE = 5000; // Maximum validation cache entries
        static constexpr size_t CLEANUP_THRESHOLD = 8000; // Start cleanup at 80% capacity
        static constexpr size_t EVICTION_BATCH_SIZE = 1000; // Evict this many at once

        // Interface storage with access tracking
        mutable std::unordered_map<std::string, std::shared_ptr<InterfaceDefinition>> interfaces;
        mutable std::unordered_map<std::string, std::chrono::steady_clock::time_point> interfaceAccessTimes;
        mutable std::list<std::string> interfaceAccessOrder; // LRU tracking
        mutable std::unordered_map<std::string, std::list<std::string>::iterator> interfaceOrderMap;

        std::shared_ptr<circularDependency::CircularDependencyDetector> dependencyDetector;

        // Bounded validation cache for performance optimization
        mutable std::unordered_set<std::string> validatedInterfaces;
        mutable std::list<std::string> validationAccessOrder; // LRU tracking for validation cache
        mutable std::unordered_map<std::string, std::list<std::string>::iterator> validationOrderMap;

    public:
       explicit  InterfaceRegistry();
        
        ~InterfaceRegistry() = default;

        // Interface registration
        void registerInterface(const std::string& name, std::shared_ptr<InterfaceDefinition> interface);

        // Interface lookup with generic type support
        std::shared_ptr<InterfaceDefinition> findInterface(const std::string& name) const;
        
        bool hasInterface(const std::string& name) const;
        
        // Get all registered interfaces
        const std::unordered_map<std::string, std::shared_ptr<InterfaceDefinition>>& getAllInterfaces() const;
        
        // Clear all interfaces (useful for testing)
        void clear();
        

        // Remove specific interface
        bool removeInterface(const std::string& name);

        // Remove interfaces by predicate (for selective cleanup)
        template <typename Predicate>
        size_t removeInterfacesIf(Predicate pred)
        {
            size_t removedCount = 0;
            std::vector<std::string> toRemove;

            // First, collect interfaces to remove
            for (auto it = interfaces.begin(); it != interfaces.end(); ++it)
            {
                if (pred(it->first, it->second))
                {
                    toRemove.push_back(it->first);
                }
            }

            // Then remove them and clean up LRU tracking
            for (const auto& name : toRemove)
            {
                interfaces.erase(name);

                // Clean up interface LRU tracking
                interfaceAccessTimes.erase(name);
                auto orderIt = interfaceOrderMap.find(name);
                if (orderIt != interfaceOrderMap.end())
                {
                    interfaceAccessOrder.erase(orderIt->second);
                    interfaceOrderMap.erase(orderIt);
                }
                removedCount++;
            }

            // Clear validation cache if any interfaces were removed
            if (removedCount > 0)
            {
                clearValidationCache();
            }
            return removedCount;
        }

        // Get interface count
        size_t count() const;
        
        // Memory management: Check for unused interfaces
        std::vector<std::string> findUnusedInterfaces() const;
        
        // Cleanup unused interfaces (those only referenced by registry)
        size_t cleanupUnusedInterfaces();
      

        // Enhanced validation with depth protection and caching
        bool validateInterfaceHierarchy(const std::string& interfaceName) const;

        // Configuration management
        void setInterfaceDependencyConfig(const circularDependency::CircularDependencyConfig& config);
        
        circularDependency::CircularDependencyConfig getInterfaceDependencyConfig() const;
        
        // Cache management
        void clearValidationCache();
        
        size_t getValidationCacheSize() const;
        

    private:
        void validateInterfaceHierarchyEnhanced(const std::string& interfaceName) const;

        // Memory management helper methods
        void updateInterfaceAccess(const std::string& name) const;
        void evictLRUInterfaces() const;
        void updateValidationAccess(const std::string& name) const;
        void evictLRUValidations() const;
        bool shouldEvictInterfaces() const;
        void performAutomaticCleanup() const;

        // Generic type helper method
        std::string extractBaseTypeName(const std::string& typeName) const;
       
    };
}
