#pragma once

#include <memory>
#include "../runtimeTypes/klass/ObjectInstance.hpp"

namespace value {

/**
 * Cache for the two Bool object instances (true/false). With only two
 * possible values the hit rate is 100% — every `new Bool(x)` returns
 * the same shared instance, eliminating the allocation entirely.
 *
 * Mirrors IntegerCache; lazily populated on first access because the
 * Bool class definition is loaded after VM startup.
 */
class BoolCache {
public:
    /**
     * Get the cached Bool object for the given value.
     * @param value The boolean value
     * @param boolClassDef The Bool class definition (needed for lazy init)
     * @return Shared pointer to cached Bool object, or nullptr if class def missing
     */
    static std::shared_ptr<runtimeTypes::klass::ObjectInstance> getBool(
        bool value,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> boolClassDef);

    static void initialize();
    static void clear();

private:
    BoolCache() = default;
    static BoolCache& getInstance();

    static std::shared_ptr<runtimeTypes::klass::ObjectInstance> createCachedBool(
        bool value,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> boolClassDef);

    std::shared_ptr<runtimeTypes::klass::ObjectInstance> falseInstance_;
    std::shared_ptr<runtimeTypes::klass::ObjectInstance> trueInstance_;
    bool initialized_ = false;
};

} // namespace value
