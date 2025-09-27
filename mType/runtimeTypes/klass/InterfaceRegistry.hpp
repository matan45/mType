#pragma once

#include "InterfaceDefinition.hpp"
#include "../../exceptions/CircularDependencyDetector.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace runtimeTypes::klass {

    class InterfaceRegistry {
    private:
        // Configuration for memory management
        static constexpr size_t MAX_INTERFACES = 10000;         // Maximum stored interfaces
        static constexpr size_t MAX_VALIDATION_CACHE = 5000;    // Maximum validation cache entries
        static constexpr size_t CLEANUP_THRESHOLD = 8000;       // Start cleanup at 80% capacity
        static constexpr size_t EVICTION_BATCH_SIZE = 1000;     // Evict this many at once

        // Interface storage with access tracking
        mutable std::unordered_map<std::string, std::shared_ptr<InterfaceDefinition>> interfaces;
        mutable std::unordered_map<std::string, std::chrono::steady_clock::time_point> interfaceAccessTimes;
        mutable std::list<std::string> interfaceAccessOrder; // LRU tracking
        mutable std::unordered_map<std::string, std::list<std::string>::iterator> interfaceOrderMap;

        std::shared_ptr<mtype::exceptions::CircularDependencyDetector> dependencyDetector;

        // Bounded validation cache for performance optimization
        mutable std::unordered_set<std::string> validatedInterfaces;
        mutable std::list<std::string> validationAccessOrder; // LRU tracking for validation cache
        mutable std::unordered_map<std::string, std::list<std::string>::iterator> validationOrderMap;

    public:
        InterfaceRegistry() {
            // Initialize enhanced circular dependency detection for interface inheritance
            mtype::exceptions::CircularDependencyConfig config;
            config.maxInterfaceDepth = 20;  // Reasonable depth for interface inheritance
            config.enableEarlyDetection = true;
            config.enablePerformanceMetrics = true;

            dependencyDetector = std::make_shared<mtype::exceptions::CircularDependencyDetector>(config);
        }
        ~InterfaceRegistry() = default;

        // Interface registration
        void registerInterface(const std::string& name, std::shared_ptr<InterfaceDefinition> interface) {
            // Check if we need to evict before adding
            if (shouldEvictInterfaces()) {
                performAutomaticCleanup();
            }

            interfaces[name] = interface;
            updateInterfaceAccess(name);

            // Clear validation cache since interface hierarchy may have changed
            clearValidationCache();
        }

        // Interface lookup with generic type support
        std::shared_ptr<InterfaceDefinition> findInterface(const std::string& name) const {
            // First try direct lookup (for non-generic interfaces)
            auto it = interfaces.find(name);
            if (it != interfaces.end()) {
                updateInterfaceAccess(name);
                return it->second;
            }

            // If not found and name contains generic parameters, try base name lookup
            if (name.find('<') != std::string::npos) {
                std::string baseName = extractBaseTypeName(name);
                auto baseIt = interfaces.find(baseName);
                if (baseIt != interfaces.end()) {
                    updateInterfaceAccess(baseName);
                    return baseIt->second;
                }
            }

            return nullptr;
        }

        bool hasInterface(const std::string& name) const {
            // First try direct lookup
            if (interfaces.find(name) != interfaces.end()) {
                return true;
            }

            // If not found and name contains generic parameters, try base name lookup
            if (name.find('<') != std::string::npos) {
                std::string baseName = extractBaseTypeName(name);
                return interfaces.find(baseName) != interfaces.end();
            }

            return false;
        }

        // Get all registered interfaces
        const std::unordered_map<std::string, std::shared_ptr<InterfaceDefinition>>& getAllInterfaces() const {
            return interfaces;
        }

        // Clear all interfaces (useful for testing)
        void clear() {
            interfaces.clear();
            interfaceAccessTimes.clear();
            interfaceAccessOrder.clear();
            interfaceOrderMap.clear();
            clearValidationCache();
        }

        // Remove specific interface
        bool removeInterface(const std::string& name) {
            auto it = interfaces.find(name);
            if (it != interfaces.end()) {
                interfaces.erase(it);

                // Clean up interface LRU tracking
                interfaceAccessTimes.erase(name);
                auto orderIt = interfaceOrderMap.find(name);
                if (orderIt != interfaceOrderMap.end()) {
                    interfaceAccessOrder.erase(orderIt->second);
                    interfaceOrderMap.erase(orderIt);
                }

                // Clear validation cache since interface structure changed
                clearValidationCache();
                return true;
            }
            return false;
        }

        // Remove interfaces by predicate (for selective cleanup)
        template<typename Predicate>
        size_t removeInterfacesIf(Predicate pred) {
            size_t removedCount = 0;
            std::vector<std::string> toRemove;

            // First, collect interfaces to remove
            for (auto it = interfaces.begin(); it != interfaces.end(); ++it) {
                if (pred(it->first, it->second)) {
                    toRemove.push_back(it->first);
                }
            }

            // Then remove them and clean up LRU tracking
            for (const auto& name : toRemove) {
                interfaces.erase(name);

                // Clean up interface LRU tracking
                interfaceAccessTimes.erase(name);
                auto orderIt = interfaceOrderMap.find(name);
                if (orderIt != interfaceOrderMap.end()) {
                    interfaceAccessOrder.erase(orderIt->second);
                    interfaceOrderMap.erase(orderIt);
                }
                removedCount++;
            }

            // Clear validation cache if any interfaces were removed
            if (removedCount > 0) {
                clearValidationCache();
            }
            return removedCount;
        }

        // Get interface count
        size_t count() const {
            return interfaces.size();
        }

        // Memory management: Check for unused interfaces
        std::vector<std::string> findUnusedInterfaces() const {
            std::vector<std::string> unused;
            for (const auto& [name, interfaceDef] : interfaces) {
                // Simple heuristic: if use_count is 1, only registry holds it
                if (interfaceDef.use_count() == 1) {
                    unused.push_back(name);
                }
            }
            return unused;
        }

        // Cleanup unused interfaces (those only referenced by registry)
        size_t cleanupUnusedInterfaces() {
            return removeInterfacesIf([](const std::string&, const std::shared_ptr<InterfaceDefinition>& def) {
                return def.use_count() == 1; // Only registry references it
            });
        }

        // Enhanced validation with depth protection and caching
        bool validateInterfaceHierarchy(const std::string& interfaceName) const {
            // Check cache first - O(1) lookup for previously validated interfaces
            if (validatedInterfaces.find(interfaceName) != validatedInterfaces.end()) {
                updateValidationAccess(interfaceName);
                return true; // Already validated successfully
            }

            // Perform validation only if not cached
            try {
                validateInterfaceHierarchyEnhanced(interfaceName);

                // Check if we need to evict before adding to cache
                if (validatedInterfaces.size() >= MAX_VALIDATION_CACHE) {
                    evictLRUValidations();
                }

                // Cache successful validation result with LRU tracking
                validatedInterfaces.insert(interfaceName);
                updateValidationAccess(interfaceName);
                return true;
            }
            catch (const mtype::exceptions::TrueCyclicException&) {
                return false;  // True circular dependency - don't cache failed validations
            }
            catch (const mtype::exceptions::DepthLimitException&) {
                return false;  // Depth limit exceeded - don't cache failed validations
            }
        }

        // Configuration management
        void setInterfaceDependencyConfig(const mtype::exceptions::CircularDependencyConfig& config) {
            dependencyDetector->setConfig(config);
            // Clear cache when configuration changes as limits may affect validation
            validatedInterfaces.clear();
        }

        mtype::exceptions::CircularDependencyConfig getInterfaceDependencyConfig() const {
            return dependencyDetector->getConfig();
        }

        // Get performance metrics
        const mtype::exceptions::DependencyDetectionMetrics& getDependencyMetrics() const {
            return dependencyDetector->getMetrics();
        }

        // Cache management
        void clearValidationCache() {
            validatedInterfaces.clear();
            validationAccessOrder.clear();
            validationOrderMap.clear();
        }

        size_t getValidationCacheSize() const {
            return validatedInterfaces.size();
        }

    private:
        void validateInterfaceHierarchyEnhanced(const std::string& interfaceName) const {
            // Use RAII helper for automatic cleanup
            mtype::exceptions::DependencyScope scope(
                *dependencyDetector,
                mtype::exceptions::DependencyType::INTERFACE_INHERITANCE,
                interfaceName,
                "interface hierarchy validation"
            );

            auto interface = findInterface(interfaceName);
            if (!interface) {
                return; // Interface not found, but that's okay for validation
            }

            // Recursively validate all extended interfaces
            for (const auto& extendedInterface : interface->getExtendedInterfaces()) {
                validateInterfaceHierarchyEnhanced(extendedInterface);
            }
        }

        // Legacy validation method for backward compatibility (deprecated)
        bool validateInterfaceHierarchyHelper(const std::string& interfaceName,
                                            std::unordered_set<std::string>& visited) const {
            // Check for cycles
            if (visited.find(interfaceName) != visited.end()) {
                return false; // Circular dependency detected
            }

            auto interface = findInterface(interfaceName);
            if (!interface) {
                return true; // Interface not found, but that's okay for validation
            }

            visited.insert(interfaceName);

            // Check all extended interfaces
            for (const auto& extendedInterface : interface->getExtendedInterfaces()) {
                if (!validateInterfaceHierarchyHelper(extendedInterface, visited)) {
                    return false;
                }
            }

            visited.erase(interfaceName);
            return true;
        }

        // Memory management helper methods
        void updateInterfaceAccess(const std::string& name) const;
        void evictLRUInterfaces() const;
        void updateValidationAccess(const std::string& name) const;
        void evictLRUValidations() const;
        bool shouldEvictInterfaces() const;
        void performAutomaticCleanup() const;

        // Generic type helper method
        std::string extractBaseTypeName(const std::string& typeName) const {
            // Extract base type name from generic instantiation (e.g., "Predicate<Person>" -> "Predicate")
            size_t anglePos = typeName.find('<');
            if (anglePos != std::string::npos) {
                return typeName.substr(0, anglePos);
            }
            return typeName;
        }
    };
}