#pragma once
// MYT-228: per-frame storage for method/free-fn generic type-arg bindings.
//
// Design goals:
//   - Zero overhead per frame when no generics are involved (CallFrame pays
//     8 bytes for a possibly-null pointer; nothing else).
//   - Zero heap allocation per generic call after warmup (a thread-local
//     pool of std::unordered_map recycles the storage).
//   - Move on the hot path (vector growth, pushCallFrame) — noexcept
//     transfer of the pool slot, no allocation.
//   - Deep copy on the snapshot-then-restore paths (interop/async) so a
//     rollback after an exception preserves any bindings captured at
//     snapshot time. Deep copy acquires a fresh pool slot per source
//     binding — paid only at known-slow interop boundaries, not in
//     tight CALL loops where everything moves.
//
// The previous representation was `std::optional<std::unordered_map<...>>`
// (~72 bytes per frame on MSVC, with a heap alloc per generic call). The
// frame-size growth measurably regressed object_alloc-style benchmarks
// where every constructor invocation pushes a CallFrame, even though no
// generics are involved.
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

namespace vm::runtime
{
    using TypeArgMap = std::unordered_map<std::string, std::string>;

    namespace detail
    {
        // Thread-local pool that owns the maps. acquire() returns a cleared
        // map; release(m) marks it as free for reuse. Pool grows on demand
        // and only shrinks at thread exit. owned_ uses unique_ptr so map
        // addresses are stable across pool growth.
        class TypeArgMapPool
        {
        public:
            TypeArgMap* acquire();
            void release(TypeArgMap* m) noexcept;

        private:
            std::vector<std::unique_ptr<TypeArgMap>> owned_;
            std::vector<TypeArgMap*> free_;
        };

        TypeArgMapPool& threadLocalTypeArgPool();
    }

    // RAII wrapper around a pool-owned TypeArgMap*.
    //
    // Hot path (frame push, vector growth): uses move semantics.
    // pushCallFrame takes a CallFrame by value and the call sites all use
    // std::move; vector::push_back / vector growth use noexcept move. No
    // allocation happens on the hot path beyond what the pool already
    // recycles.
    //
    // Cold path (interop/async snapshot-then-restore patterns —
    // VirtualMachineInteropCall, VirtualMachineAsync, etc.): uses deep
    // copy. The snapshot needs to preserve any bindings present at
    // capture time so a rollback after an exception can restore the
    // caller's pre-interop state. Each copy acquires a fresh pool slot
    // and clones the map contents — paid only at known-slow boundaries,
    // not in tight CALL loops.
    //
    // [[nodiscard]] on releasePtr() so accidentally discarding the
    // returned pointer (which leaks the pool slot) is a compile-time
    // warning instead of a silent leak.
    class TypeArgMapPtr
    {
    public:
        TypeArgMapPtr() noexcept = default;

        // Deep copy: clone map contents into a fresh pool slot. Used by
        // the snapshot/restore paths in interop and async code; never
        // hit on the hot CALL path (callers there std::move).
        TypeArgMapPtr(const TypeArgMapPtr& other)
        {
            if (other.ptr_)
            {
                ptr_ = detail::threadLocalTypeArgPool().acquire();
                *ptr_ = *other.ptr_;
            }
        }
        TypeArgMapPtr& operator=(const TypeArgMapPtr& other)
        {
            if (this != &other)
            {
                release();
                if (other.ptr_)
                {
                    ptr_ = detail::threadLocalTypeArgPool().acquire();
                    *ptr_ = *other.ptr_;
                }
            }
            return *this;
        }

        TypeArgMapPtr(TypeArgMapPtr&& other) noexcept : ptr_(other.ptr_)
        {
            other.ptr_ = nullptr;
        }
        TypeArgMapPtr& operator=(TypeArgMapPtr&& other) noexcept
        {
            if (this != &other)
            {
                release();
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
            }
            return *this;
        }

        ~TypeArgMapPtr() noexcept { release(); }

        explicit operator bool() const noexcept { return ptr_ != nullptr; }
        TypeArgMap* get() const noexcept { return ptr_; }
        TypeArgMap& operator*() const noexcept { return *ptr_; }
        TypeArgMap* operator->() const noexcept { return ptr_; }

        // Acquire a fresh (cleared) map from the pool, releasing any
        // existing one first. Returns a reference to the now-owned map
        // for in-place population.
        TypeArgMap& acquireFresh()
        {
            release();
            ptr_ = detail::threadLocalTypeArgPool().acquire();
            return *ptr_;
        }

        // Hand off ownership to a raw pointer; this wrapper becomes empty.
        // [[nodiscard]] — discarding the returned pointer leaks the pool slot.
        [[nodiscard]] TypeArgMap* releasePtr() noexcept
        {
            auto* p = ptr_;
            ptr_ = nullptr;
            return p;
        }

        // Take ownership of an externally-acquired pointer (e.g. one
        // released from a sibling wrapper). Releases any existing pointer
        // back to the pool first.
        void adopt(TypeArgMap* p) noexcept
        {
            release();
            ptr_ = p;
        }

        // Eagerly release back to the pool; equivalent to assigning a
        // default-constructed TypeArgMapPtr.
        void reset() noexcept { release(); }

    private:
        void release() noexcept
        {
            if (ptr_)
            {
                detail::threadLocalTypeArgPool().release(ptr_);
                ptr_ = nullptr;
            }
        }

        TypeArgMap* ptr_ = nullptr;
    };
}
