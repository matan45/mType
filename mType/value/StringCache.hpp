#pragma once

#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include <cstddef>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>

namespace value
{
    // MYT-272: cache wrapper layer for `String` ObjectInstances. The
    // underlying string content is already deduplicated by StringPool;
    // this cache adds a per-id wrapper-instance layer on top, so
    // `new String("foo")` returns the same ObjectInstance across calls.
    //
    // Eligibility:
    //   - empty string ("") is cached as a singleton (StringPool excludes
    //     len < MIN_INTERN_LENGTH, so its id can't be used as a key).
    //   - non-empty strings cache iff StringPool agrees to intern them
    //     (in-range length); cache key is the interned poolId.
    //   - cap at kMaxEntries with FIFO eviction when full so unbounded
    //     workloads don't grow the wrapper cache without bound.
    //
    // String is a `value class`, so identity reuse is safe.
    class StringCache
    {
    public:
        static std::shared_ptr<runtimeTypes::klass::ObjectInstance> getString(
            const std::string& value,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> stringClassDef);

        static void initialize();
        static void clear();

        static constexpr size_t kMaxEntries = 256;

    private:
        StringCache() = default;
        static StringCache& getInstance();

        static std::shared_ptr<runtimeTypes::klass::ObjectInstance> createCachedString(
            const std::string& value,
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> stringClassDef);

        std::shared_ptr<runtimeTypes::klass::ObjectInstance> emptyInstance_;
        std::unordered_map<size_t, std::shared_ptr<runtimeTypes::klass::ObjectInstance>> cache_;
        std::deque<size_t> insertOrder_;
        bool initialized_ = false;
    };
}
