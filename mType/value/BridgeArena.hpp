#pragma once
//
// BridgeArena — per-BridgeKind freelist for TypedBridge<K, Held> allocations
// (MYT-317 sub-item 1).
//
// Every heap Value owns a TypedBridge<K, Held>; pre-pool each Value creation
// paid one extra allocation for that bridge. The arena caches destructed raw
// memory blocks keyed by BridgeKind so warm paths skip the allocator.
//
// Threading: single-threaded VM (same regime as IntrusivePtr / ObjectInstancePool).
//
// Lifecycle: BridgeBase::destroy() (overriding RefCounted::destroy()) runs the
// derived TypedBridge destructor first, then hands the empty memory block here.
// The freelist holds NO live Held value at rest, so reset() is a plain free()
// loop with no destructor work — safe to call on plugin unload without leaving
// plugin-owned shared_ptrs dangling.
//
// Header-only so the LSP build (which links ValueTypeUtils.cpp but no other
// arena .cpp) doesn't need a premake entry.
//

#include "ValueBridge.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <vector>

namespace value
{
    struct BridgeArenaStats
    {
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t discards = 0;
        uint64_t resets = 0;
        size_t   cachedSlots = 0;
    };

    class BridgeArena
    {
    public:
        static constexpr size_t KIND_COUNT = 10;
        static constexpr size_t HOT_CAP    = 32;
        static constexpr size_t COLD_CAP   = 8;

        static BridgeArena& getInstance() noexcept
        {
            // Intentional leak — must outlive any Value whose destructor
            // funnels through BridgeBase::destroy() back into this pool.
            static BridgeArena* instance = new BridgeArena();
            return *instance;
        }

        void* acquireSlot(BridgeKind k, size_t /*bytes*/) noexcept
        {
            auto& bucket = buckets_[static_cast<size_t>(k)];
            if (!bucket.slots.empty())
            {
                void* mem = bucket.slots.back();
                bucket.slots.pop_back();
                --stats_.cachedSlots;
                ++stats_.hits;
                return mem;
            }
            ++stats_.misses;
            return nullptr;
        }

        void releaseSlot(BridgeKind k, void* mem) noexcept
        {
            if (!mem) return;
            auto& bucket = buckets_[static_cast<size_t>(k)];
            if (bucket.slots.size() < bucket.cap)
            {
                bucket.slots.push_back(mem);
                ++stats_.cachedSlots;
                return;
            }
            ++stats_.discards;
            freeAligned(mem);
        }

        void reset() noexcept
        {
            for (auto& bucket : buckets_)
            {
                for (void* mem : bucket.slots)
                {
                    freeAligned(mem);
                }
                bucket.slots.clear();
            }
            stats_.cachedSlots = 0;
            ++stats_.resets;
        }

        BridgeArenaStats getStats() const noexcept { return stats_; }

    private:
        struct Bucket
        {
            std::vector<void*> slots;
            size_t cap = COLD_CAP;
        };

        BridgeArena() noexcept
        {
            // Hot kinds (32). The four most-allocated bridges per the
            // allocation-heavy workload survey.
            buckets_[static_cast<size_t>(BridgeKind::STD_STRING)].cap      = HOT_CAP;
            buckets_[static_cast<size_t>(BridgeKind::OBJECT_INSTANCE)].cap = HOT_CAP;
            buckets_[static_cast<size_t>(BridgeKind::BYTECODE_LAMBDA)].cap = HOT_CAP;
            buckets_[static_cast<size_t>(BridgeKind::NATIVE_ARRAY)].cap    = HOT_CAP;
            // Cold kinds (8) — initialized to COLD_CAP by Bucket default.
        }

        BridgeArena(const BridgeArena&) = delete;
        BridgeArena& operator=(const BridgeArena&) = delete;

        static void freeAligned(void* mem) noexcept
        {
            ::operator delete(mem, std::align_val_t{alignof(std::max_align_t)});
        }

        std::array<Bucket, KIND_COUNT> buckets_;
        BridgeArenaStats stats_;
    };
}
