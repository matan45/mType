#include "InterfaceRegistry.hpp"
#include <unordered_set>
#include <chrono>

namespace runtimeTypes::klass {

    void InterfaceRegistry::updateInterfaceAccess(const std::string& name) const {
        // Update access time
        interfaceAccessTimes[name] = std::chrono::steady_clock::now();

        // Move to front of LRU list (most recently used)
        auto orderIt = interfaceOrderMap.find(name);
        if (orderIt != interfaceOrderMap.end()) {
            interfaceAccessOrder.erase(orderIt->second);
        }
        interfaceAccessOrder.push_front(name);
        interfaceOrderMap[name] = interfaceAccessOrder.begin();
    }

    void InterfaceRegistry::evictLRUInterfaces() const {
        // Evict least recently used interfaces until we're under the cleanup threshold
        while (interfaces.size() > CLEANUP_THRESHOLD && !interfaceAccessOrder.empty()) {
            const std::string& lruInterface = interfaceAccessOrder.back();

            // Only evict if use_count is 1 (only registry holds reference)
            auto it = interfaces.find(lruInterface);
            if (it != interfaces.end() && it->second.use_count() == 1) {
                interfaces.erase(it);
                interfaceAccessTimes.erase(lruInterface);
                interfaceOrderMap.erase(lruInterface);
                interfaceAccessOrder.pop_back();
            } else {
                // Can't evict this one, move to front and try next
                interfaceAccessOrder.pop_back();
                interfaceAccessOrder.push_front(lruInterface);
                interfaceOrderMap[lruInterface] = interfaceAccessOrder.begin();
                break; // Avoid infinite loop if all interfaces are in use
            }
        }
    }

    void InterfaceRegistry::updateValidationAccess(const std::string& name) const {
        // Move to front of validation cache LRU list
        auto orderIt = validationOrderMap.find(name);
        if (orderIt != validationOrderMap.end()) {
            validationAccessOrder.erase(orderIt->second);
        }
        validationAccessOrder.push_front(name);
        validationOrderMap[name] = validationAccessOrder.begin();
    }

    void InterfaceRegistry::evictLRUValidations() const {
        // Evict validation cache entries until we're under the limit
        while (validatedInterfaces.size() > static_cast<size_t>(MAX_VALIDATION_CACHE * 0.8) &&
               !validationAccessOrder.empty()) {
            const std::string& lruValidation = validationAccessOrder.back();
            validatedInterfaces.erase(lruValidation);
            validationOrderMap.erase(lruValidation);
            validationAccessOrder.pop_back();
        }
    }

    bool InterfaceRegistry::shouldEvictInterfaces() const {
        return interfaces.size() >= MAX_INTERFACES ||
               (interfaces.size() >= CLEANUP_THRESHOLD &&
                interfaceAccessOrder.size() >= EVICTION_BATCH_SIZE);
    }

    void InterfaceRegistry::performAutomaticCleanup() const {
        // First try to clean up unused interfaces
        size_t unusedRemoved = const_cast<InterfaceRegistry*>(this)->cleanupUnusedInterfaces();

        // If we still need space, evict LRU interfaces
        if (interfaces.size() >= CLEANUP_THRESHOLD) {
            evictLRUInterfaces();
        }

        // Also clean up validation cache if needed
        if (validatedInterfaces.size() >= MAX_VALIDATION_CACHE) {
            evictLRUValidations();
        }
    }

}