#include "IntegerCache.hpp"
#include <cstddef>
#include <cstdint>
#include "../environment/Environment.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"

namespace value {

// Singleton instance with thread-safe initialization
IntegerCache& IntegerCache::getInstance() {
    static IntegerCache instance;
    return instance;
}

void IntegerCache::initialize() {
    // Note: Cannot pre-initialize here because Int class may not be loaded yet
    // Cache will be lazily populated on first access
    auto& instance = getInstance();
    instance.initialized_ = true;
}

std::shared_ptr<runtimeTypes::klass::ObjectInstance> IntegerCache::createCachedInt(
    int64_t value,
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> intClassDef)
{
    if (!intClassDef) {
        return nullptr;
    }

    // Create Int object instance
    auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(
        intClassDef,
        std::unordered_map<std::string, std::string>{}  // No generic bindings
    );

    // Initialize the 'value' field with the cached integer value
    // Int class has a private 'value' field of type int64_t
    instance->setField("value", value);

    return instance;
}

std::shared_ptr<runtimeTypes::klass::ObjectInstance> IntegerCache::getInt(
    int64_t value,
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> intClassDef)
{
    auto& instance = getInstance();

    // Ensure cache is marked as initialized
    if (!instance.initialized_) {
        initialize();
    }

    // Check if value is in cacheable range
    if (value >= CACHE_MIN && value <= CACHE_MAX) {
        size_t index = value - CACHE_MIN;

        // Lazy initialization: create cached object on first access
        if (!instance.cache_[index] && intClassDef) {
            instance.cache_[index] = createCachedInt(value, intClassDef);
        }

        auto cached = instance.cache_[index];
        if (cached) {
            return cached;  // Return cached instance
        }
    }

    // Value outside cache range - return nullptr to signal caller to create normally
    return nullptr;
}

void IntegerCache::clear() {
    auto& instance = getInstance();
    instance.cache_.fill(nullptr);
    instance.initialized_ = false;
}

} // namespace value
