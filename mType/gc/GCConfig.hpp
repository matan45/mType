#pragma once

#include <cstddef>
#include <cstdint>

namespace gc
{
    namespace config
    {
        // Collection triggers
        constexpr size_t ALLOCATION_THRESHOLD = 10000;      // Trigger after N allocations
        constexpr size_t SUSPECT_THRESHOLD = 1000;          // Trigger when suspect list reaches N
        constexpr size_t MEMORY_THRESHOLD_MB = 100;         // Trigger at N MB heap usage

        // Collection limits
        constexpr size_t MAX_CYCLE_DETECTION_TIME_MS = 50;   // Abort if taking too long
        constexpr size_t MAX_OBJECTS_PER_CYCLE = 10000;     // Limit objects processed per cycle
        constexpr size_t MAX_TRAVERSAL_DEPTH = 10000;       // Max depth for iterative traversal stack

        // Suspect buffer configuration
        constexpr size_t SUSPECT_BUFFER_INITIAL_SIZE = 256;
        constexpr size_t SUSPECT_BUFFER_MAX_SIZE = 100000;

        // VM integration
        constexpr size_t GC_CHECK_INTERVAL = 1000;          // Instructions between GC checks

        // Object colors for cycle detection (Bacon algorithm)
        enum class ObjectColor : uint8_t
        {
            BLACK = 0,    // Definitely reachable
            WHITE = 1,    // Potentially garbage (candidate for collection)
            GRAY = 2,     // In process of being scanned
            PURPLE = 3    // Potential cycle root (refcount decremented but not zero)
        };

        // Object type tags for polymorphic traversal
        enum class GCObjectType : uint8_t
        {
            UNKNOWN = 0,
            OBJECT_INSTANCE = 1,
            BYTECODE_LAMBDA = 2,
            NATIVE_ARRAY = 3,
            FLAT_MULTI_ARRAY = 4,
            SPARSE_MULTI_ARRAY = 5,
            PROMISE_VALUE = 6
        };
    }
}
