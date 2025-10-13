#pragma once
#include "../base/IArray.hpp"
#include "../../simd/SIMDConfig.hpp"
#include <vector>
#include <memory>
#include <cstdint>
#include <stdexcept>

namespace mType {
namespace value {
namespace arrays {

/**
 * Template base class for primitive arrays (int, float, bool).
 * Uses SIMD when available, falls back to scalar operations.
 *
 * Design Principles:
 * - Template Method Pattern: Base class defines algorithm structure
 * - Strategy Pattern: SIMD vs scalar implementation selection
 * - RAII: Automatic memory management
 */
template<typename T>
class PrimitiveArray : public IArray {
public:
    static_assert(
        std::is_same<T, int>::value ||
        std::is_same<T, int32_t>::value ||
        std::is_same<T, float>::value ||
        std::is_same<T, bool>::value,
        "PrimitiveArray only supports int, int32_t, float, and bool"
    );

    // Construction
    explicit PrimitiveArray(size_t initialSize = 0)
        : data_(initialSize), aligned_(false) {
        if (initialSize >= mType::value::simd::SIMDThreshold::MIN_ELEMENTS) {
            ensureAlignment();
        }
    }

    explicit PrimitiveArray(const std::vector<T>& data)
        : data_(data), aligned_(false) {
        if (data_.size() >= mType::value::simd::SIMDThreshold::MIN_ELEMENTS) {
            ensureAlignment();
        }
    }

    explicit PrimitiveArray(std::vector<T>&& data)
        : data_(std::move(data)), aligned_(false) {
        if (data_.size() >= mType::value::simd::SIMDThreshold::MIN_ELEMENTS) {
            ensureAlignment();
        }
    }

    // IArray implementation
    size_t size() const override { return data_.size(); }
    size_t capacity() const override { return data_.capacity(); }
    bool empty() const override { return data_.empty(); }

    std::string elementTypeName() const override {
        if (std::is_same<T, int>::value || std::is_same<T, int32_t>::value) return "int";
        if (std::is_same<T, float>::value) return "float";
        if (std::is_same<T, bool>::value) return "bool";
        return "unknown";
    }

    ::value::ValueType elementType() const override {
        if (std::is_same<T, int>::value || std::is_same<T, int32_t>::value) return ::value::ValueType::INT;
        if (std::is_same<T, float>::value) return ::value::ValueType::FLOAT;
        if (std::is_same<T, bool>::value) return ::value::ValueType::BOOL;
        return ::value::ValueType::VOID;
    }

    ::value::Value get(size_t index) const override {
        if (index >= data_.size()) {
            throw std::out_of_range("Array index out of bounds");
        }
        return ::value::Value(data_[index]);
    }

    void set(size_t index, const ::value::Value& value) override {
        if (index >= data_.size()) {
            throw std::out_of_range("Array index out of bounds");
        }

        if (std::holds_alternative<T>(value)) {
            data_[index] = std::get<T>(value);
        } else {
            throw std::runtime_error("Type mismatch in array assignment");
        }
    }

    void reserve(size_t newCapacity) override {
        data_.reserve(newCapacity);
        if (newCapacity >= mType::value::simd::SIMDThreshold::MIN_ELEMENTS) {
            ensureAlignment();
        }
    }

    void resize(size_t newSize) override {
        data_.resize(newSize);
        if (newSize >= mType::value::simd::SIMDThreshold::MIN_ELEMENTS) {
            ensureAlignment();
        }
    }

    void clear() override {
        data_.clear();
        aligned_ = false;
    }

    bool supportsSIMD() const override {
#ifdef MTYPE_SIMD_ENABLED
        return data_.size() >= mType::value::simd::SIMDThreshold::MIN_ELEMENTS;
#else
        return false;
#endif
    }

    size_t simdWidth() const override {
#ifdef MTYPE_SIMD_ENABLED
        if (std::is_same<T, int>::value || std::is_same<T, int32_t>::value) return mType::value::simd::SIMDWidth::INT32;
        if (std::is_same<T, float>::value) return mType::value::simd::SIMDWidth::FLOAT;
#endif
        return 1;
    }

    std::unique_ptr<IArray> clone() const override {
        return std::make_unique<PrimitiveArray<T>>(data_);
    }

    // Direct access for SIMD operations
    // Note: std::vector<bool> doesn't provide .data(), so these are only for int/float
    T* data() {
        static_assert(!std::is_same<T, bool>::value,
            "data() not available for bool - use get/set instead");
        return data_.data();
    }

    const T* data() const {
        static_assert(!std::is_same<T, bool>::value,
            "data() not available for bool - use get/set instead");
        return data_.data();
    }

    // Aligned allocation for SIMD efficiency
    bool isAligned() const { return aligned_; }

    void ensureAlignment() {
        // std::vector<bool> is a special case - it's bit-packed and doesn't provide .data()
        // For bool arrays, we can't check alignment, so just mark as not aligned
        if constexpr (std::is_same<T, bool>::value) {
            aligned_ = false;
        } else {
            // Check if data is already aligned
            uintptr_t addr = reinterpret_cast<uintptr_t>(data_.data());
            if (addr % mType::value::simd::SIMDAlignment::BYTES == 0) {
                aligned_ = true;
            } else {
                aligned_ = false;
                // Note: std::vector doesn't guarantee alignment beyond alignof(T)
                // For production use, would need custom allocator
            }
        }
    }

private:
    std::vector<T> data_;
    bool aligned_;
};

// Type aliases for clarity
// Note: Using 'int' to match the Value variant type (line 45 in ValueType.hpp)
// On most platforms, int is 32-bit (same as int32_t)
using IntArray = PrimitiveArray<int>;
using FloatArray = PrimitiveArray<float>;
using BoolArray = PrimitiveArray<bool>;

} // namespace arrays
} // namespace value
} // namespace mType
