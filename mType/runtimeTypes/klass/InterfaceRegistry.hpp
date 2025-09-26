#pragma once

#include "InterfaceDefinition.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace runtimeTypes::klass {

    class InterfaceRegistry {
    private:
        std::unordered_map<std::string, std::shared_ptr<InterfaceDefinition>> interfaces;

    public:
        InterfaceRegistry() = default;
        ~InterfaceRegistry() = default;

        // Interface registration
        void registerInterface(const std::string& name, std::shared_ptr<InterfaceDefinition> interface) {
            interfaces[name] = interface;
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
        }

        // Get interface count
        size_t count() const {
            return interfaces.size();
        }

        // Validate interface inheritance chains (prevent cycles)
        bool validateInterfaceHierarchy(const std::string& interfaceName) const {
            std::unordered_set<std::string> visited;
            return validateInterfaceHierarchyHelper(interfaceName, visited);
        }

    private:
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