#pragma once

#include <memory>
#include <array>
#include "../runtimeTypes/klass/ObjectInstance.hpp"

namespace value {

/**
 * Cache for commonly-used Integer objects to reduce allocations.
 * Similar to Java's Integer cache (-128 to 127) and Python's integer pooling.
 *
 * This is part of Phase 2: Performance Optimizations for Pure OOP.
 * Pre-allocates Int objects for the range [-128, 256] which covers:
 * - Small loop counters (0-100)
 * - Common status codes
 * - Array indices
 * - Boolean-like values (0, 1)
 */
class IntegerCache {
public:
    // Cache range: -128 to 256 (385 total objects)
    static constexpr int64_t CACHE_MIN = -128;
    static constexpr int64_t CACHE_MAX = 256;
    static constexpr size_t CACHE_SIZE = CACHE_MAX - CACHE_MIN + 1;

    /**
     * Get a cached Int object if value is in range, otherwise return nullptr
     * @param value The integer value
     * @param intClassDef The Int class definition (needed for lazy initialization)
     * @return Shared pointer to cached Int object, or nullptr if outside cache range
     */
    static std::shared_ptr<runtimeTypes::klass::ObjectInstance> getInt(
        int64_t value,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> intClassDef);

    /**
     * Check if a value is in the cacheable range
     */
    static bool isCacheable(int64_t value) {
        return value >= CACHE_MIN && value <= CACHE_MAX;
    }

    /**
     * Initialize the cache (called once at startup)
     */
    static void initialize();

    /**
     * Clear the cache (for testing/shutdown)
     */
    static void clear();

private:
    // Singleton pattern
    IntegerCache() = default;
    static IntegerCache& getInstance();

    // Cache storage
    std::array<std::shared_ptr<runtimeTypes::klass::ObjectInstance>, CACHE_SIZE> cache_;
    bool initialized_ = false;

    // Create a single cached Int object with the given value
    static std::shared_ptr<runtimeTypes::klass::ObjectInstance> createCachedInt(
        int64_t value,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> intClassDef);
};

} // namespace value
