#pragma once
// MYT-228: per-frame storage for method/free-fn generic type-arg bindings.
//
// Design goals:
//   - Zero overhead per frame when no generics are involved (CallFrame pays
//     8 bytes for a possibly-null pointer; nothing else).
//   - Zero heap allocation per generic call after warmup (a thread-local
//     pool of std::unordered_map recycles the storage).
//   - Default copy/move semantics that play nicely with vector<CallFrame>:
//     copy nulls the destination (the convention is that only the in-vector
//     frame owns the map — local CallFrame instances built by callers
//     never own one), move transfers ownership and nulls the source.
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
    // Copy semantics are asymmetric on purpose: copy-construct yields a
    // null destination (intentional — see header doc above). This matches
    // how CallFrame is used: callers build local frames with no bindings,
    // pushCallFrame copies them into the vector (yielding null in the
    // vector entry), then attaches pendingTypeArgs after the copy. The
    // source local CallFrame retains its null pointer.
    //
    // Move semantics transfer ownership and null the source. std::vector
    // growth uses move on noexcept-move-ctor types (which we are), so the
    // map pointer stays attached to the right frame across vector growth.
    class TypeArgMapPtr
    {
    public:
        TypeArgMapPtr() noexcept = default;

        TypeArgMapPtr(const TypeArgMapPtr&) noexcept : ptr_(nullptr) {}
        TypeArgMapPtr& operator=(const TypeArgMapPtr&) noexcept
        {
            release();
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
        TypeArgMap* releasePtr() noexcept
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
