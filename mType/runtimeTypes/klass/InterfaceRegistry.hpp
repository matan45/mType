#pragma once

#include "InterfaceDefinition.hpp"
#include "../../exceptions/CircularDependencyDetector.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace runtimeTypes::klass {

    class InterfaceRegistry {
    private:
        std::unordered_map<std::string, std::shared_ptr<InterfaceDefinition>> interfaces;
        std::shared_ptr<mtype::exceptions::CircularDependencyDetector> dependencyDetector;

        // Validation cache for performance optimization
        mutable std::unordered_set<std::string> validatedInterfaces;

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
            interfaces[name] = interface;
            // Clear validation cache since interface hierarchy may have changed
            validatedInterfaces.clear();
        }

        // Interface lookup
        std::shared_ptr<InterfaceDefinition> findInterface(const std::string& name) const {
            auto it = interfaces.find(name);
            return (it != interfaces.end()) ? it->second : nullptr;
        }

        bool hasInterface(const std::string& name) const {
            return interfaces.find(name) != interfaces.end();
        }

        // Get all registered interfaces
        const std::unordered_map<std::string, std::shared_ptr<InterfaceDefinition>>& getAllInterfaces() const {
            return interfaces;
        }

        // Clear all interfaces (useful for testing)
        void clear() {
            interfaces.clear();
            validatedInterfaces.clear();
        }

        // Remove specific interface
        bool removeInterface(const std::string& name) {
            auto it = interfaces.find(name);
            if (it != interfaces.end()) {
                interfaces.erase(it);
                // Clear validation cache since interface structure changed
                validatedInterfaces.clear();
                return true;
            }
            return false;
        }

        // Remove interfaces by predicate (for selective cleanup)
        template<typename Predicate>
        size_t removeInterfacesIf(Predicate pred) {
            size_t removedCount = 0;
            auto it = interfaces.begin();
            while (it != interfaces.end()) {
                if (pred(it->first, it->second)) {
                    it = interfaces.erase(it);
                    removedCount++;
                } else {
                    ++it;
                }
            }
            // Clear validation cache if any interfaces were removed
            if (removedCount > 0) {
                validatedInterfaces.clear();
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
                return true; // Already validated successfully
            }

            // Perform validation only if not cached
            try {
                validateInterfaceHierarchyEnhanced(interfaceName);
                // Cache successful validation result
                validatedInterfaces.insert(interfaceName);
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
    };
}