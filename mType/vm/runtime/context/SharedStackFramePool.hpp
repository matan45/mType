#pragma once
//
// SharedStackFramePool — freelist for SharedStackFrame (MYT-317 sub-item 2).
//
// Pre-pool, every lambda invocation (and a few related sites) paid one
// std::make_shared<SharedStackFrame>(). Frames are shape-agnostic — the
// locals vector and nameToSlot map grow on demand — so a single freelist
// suffices.
//
// On release: locals.clear() / nameToSlot.clear() keep their bucket arrays
// for reuse; parentFrame.reset() releases the parent (may cascade harmlessly
// back into this pool). If locals.capacity() exceeded LOCALS_KEEP_THRESHOLD
// the slot is discarded rather than parked with permanently-bloated storage.
//
// Threading: single-threaded VM (same regime as ObjectInstancePool).
//
// Header-only for the same reason as BridgeArena — every call site that
// creates frames stays in sync without needing a separate premake entry.
//

#include "ExecutionContext.hpp"
#include "../../../value/ObjectInstancePool.hpp"  // reuse ObjectPoolStats

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace vm::runtime
{
    class SharedStackFramePool
    {
    public:
        static constexpr size_t BUCKET_CAP             = 64;
        static constexpr size_t LOCALS_KEEP_THRESHOLD  = 256;

        static SharedStackFramePool& getInstance() noexcept
        {
            // Intentional leak — same rationale as ObjectInstancePool.
            static SharedStackFramePool* instance = new SharedStackFramePool();
            return *instance;
        }

        std::shared_ptr<SharedStackFrame> acquire()
        {
            ++stats_.totalAllocations;
            SharedStackFrame* p = popOrNull();
            if (!p)
            {
                p = new SharedStackFrame();
                ++stats_.poolMisses;
            }
            else
            {
                ++stats_.poolHits;
            }
            return std::shared_ptr<SharedStackFrame>(p, SlotDeleter{this});
        }

        ::value::ObjectPoolStats getStats() const noexcept { return stats_; }

        void clear() noexcept
        {
            for (auto* p : slots_) delete p;
            slots_.clear();
            stats_.currentPoolSize = 0;
        }

    private:
        struct SlotDeleter
        {
            SharedStackFramePool* pool;
            void operator()(SharedStackFrame* p) const noexcept
            {
                if (!p) return;
                // Reset state but keep container capacities for reuse.
                if (p->locals.capacity() > LOCALS_KEEP_THRESHOLD)
                {
                    p->locals = std::vector<::value::Value>{};
                }
                else
                {
                    p->locals.clear();
                }
                p->nameToSlot.clear();
                p->parentFrame.reset();
                pool->pushOrDestroy(p);
            }
        };

        SharedStackFrame* popOrNull() noexcept
        {
            if (slots_.empty()) return nullptr;
            SharedStackFrame* p = slots_.back();
            slots_.pop_back();
            stats_.currentPoolSize = slots_.size();
            return p;
        }

        void pushOrDestroy(SharedStackFrame* p) noexcept
        {
            if (slots_.size() < BUCKET_CAP)
            {
                slots_.push_back(p);
                ++stats_.poolReturns;
                stats_.currentPoolSize = slots_.size();
                if (stats_.currentPoolSize > stats_.maxPoolSize)
                {
                    stats_.maxPoolSize = stats_.currentPoolSize;
                }
                return;
            }
            ++stats_.poolDiscards;
            delete p;
        }

        SharedStackFramePool() = default;
        SharedStackFramePool(const SharedStackFramePool&) = delete;
        SharedStackFramePool& operator=(const SharedStackFramePool&) = delete;

        std::vector<SharedStackFrame*> slots_;
        ::value::ObjectPoolStats stats_;
    };

    inline std::shared_ptr<SharedStackFrame> makePooledFrame()
    {
        return SharedStackFramePool::getInstance().acquire();
    }
}
