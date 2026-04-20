#pragma once

#include <cstddef>
#include <memory>
#include <span>

#include "ValueType.hpp"
#include "../constants/CallConstants.hpp"

namespace value
{
    // Small-buffer-optimized container for method/function dispatch args.
    //
    // MSVC's std::vector has no SBO, so constructing a vector of even one
    // element pays a heap allocation + free per call. On a dispatch-heavy
    // workload (recursive.mt, method_dispatch.mt) that's a measurable tax.
    //
    // SmallArgsBuffer keeps the first INLINE slots in-object (default-
    // constructed Values, noexcept) and spills larger sizes to a
    // unique_ptr<Value[]>. The pop-then-assign access pattern used by
    // executors (args[i] = stackManager->pop()) is unchanged — data() /
    // operator[] / span() / iterators all hit the active storage via data_.
    //
    // Non-copyable / non-movable by design: this is a per-dispatch scratch
    // buffer whose lifetime is tied to a single executor invocation.
    // Consumers should receive std::span<const Value> or std::span<Value>
    // and must not cache the span past the buffer's scope.
    class SmallArgsBuffer final
    {
    public:
        static constexpr size_t INLINE = constants::call::INLINE_ARGS_CAPACITY;

        explicit SmallArgsBuffer(size_t n) : size_(n)
        {
            if (n > INLINE)
            {
                spill_ = std::make_unique<Value[]>(n);
                data_ = spill_.get();
            }
            else
            {
                data_ = inline_;
            }
        }

        ~SmallArgsBuffer() = default;

        SmallArgsBuffer(const SmallArgsBuffer&) = delete;
        SmallArgsBuffer& operator=(const SmallArgsBuffer&) = delete;
        SmallArgsBuffer(SmallArgsBuffer&&) = delete;
        SmallArgsBuffer& operator=(SmallArgsBuffer&&) = delete;

        Value& operator[](size_t i) noexcept { return data_[i]; }
        const Value& operator[](size_t i) const noexcept { return data_[i]; }

        size_t size() const noexcept { return size_; }
        bool empty() const noexcept { return size_ == 0; }

        Value* data() noexcept { return data_; }
        const Value* data() const noexcept { return data_; }

        std::span<Value> span() noexcept { return {data_, size_}; }
        std::span<const Value> span() const noexcept { return {data_, size_}; }

        Value* begin() noexcept { return data_; }
        Value* end() noexcept { return data_ + size_; }
        const Value* begin() const noexcept { return data_; }
        const Value* end() const noexcept { return data_ + size_; }

    private:
        Value inline_[INLINE];
        std::unique_ptr<Value[]> spill_;
        Value* data_;
        size_t size_;
    };
}
