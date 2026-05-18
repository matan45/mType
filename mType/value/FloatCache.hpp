#pragma once

#include "../value/ObjectInstance.hpp"
#include <cstddef>
#include <array>
#include <cstdint>
#include <memory>

namespace value
{
    // MYT-272: hand-picked common-constant cache for Float wrapper objects.
    // Uses bitwise compare so NaN (which is never == NaN) never matches a
    // cached entry, and so +0.0 is distinguished from -0.0 (only +0.0 is
    // cached). Mirrors IntegerCache / BoolCache: lazily populated since the
    // Float class definition is loaded after VM startup.
    //
    // Threading: assumes the single-threaded VM contract — `cache_` and
    // `initialized_` are mutated without synchronization. If async work
    // is ever scheduled to a worker pool, gate population behind a mutex.
    class FloatCache
    {
    public:
        static std::shared_ptr<runtimeTypes::klass::ObjectInstance> getFloat(
            double value,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> floatClassDef);

        static bool isCacheable(double value);

        static void initialize();
        static void clear();

    private:
        static constexpr size_t CACHE_SIZE = 6;
        static const std::array<double, CACHE_SIZE> kCachedValues;

        FloatCache() = default;
        static FloatCache& getInstance();

        static std::shared_ptr<runtimeTypes::klass::ObjectInstance> createCachedFloat(
            double value,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> floatClassDef);
        static int cacheIndexOf(double value);

        std::array<std::shared_ptr<runtimeTypes::klass::ObjectInstance>, CACHE_SIZE> cache_;
        bool initialized_ = false;
    };
}
