#pragma once
#include "../base/IArray.hpp"
#include <cstddef>
#include "../../simd/SIMDConfig.hpp"
#include <vector>
#include <memory>
#include <cstdint>
#include <stdexcept>

namespace mType
{
    namespace value
    {
        namespace arrays
        {
            /**
             * Template base class for primitive arrays (int, float, bool).
             * Uses SIMD when available, falls back to scalar operations.
             *
             * Design Principles:
             * - Template Method Pattern: Base class defines algorithm structure
             * - Strategy Pattern: SIMD vs scalar implementation selection
             * - RAII: Automatic memory management
             */
            template <typename T>
            class PrimitiveArray : public IArray
            {
            public:
                static_assert(
                    std::is_same<T, int>::value ||
                    std::is_same<T, int32_t>::value ||
                    std::is_same<T, int64_t>::value ||
                    std::is_same<T, double>::value ||
                    std::is_same<T, bool>::value,
                    "PrimitiveArray only supports int, int32_t, int64_t, double, and bool"
                );

                // Construction
                explicit PrimitiveArray(size_t initialSize = 0)
                    : data_(initialSize)
                {
                }

                explicit PrimitiveArray(const std::vector<T>& data)
                    : data_(data)
                {
                }

                explicit PrimitiveArray(std::vector<T>&& data)
                    : data_(std::move(data))
                {
                }

                // IArray implementation
                size_t size() const override { return data_.size(); }
                size_t capacity() const override { return data_.capacity(); }
                bool empty() const override { return data_.empty(); }

                std::string elementTypeName() const override
                {
                    if (std::is_same<T, int>::value || std::is_same<T, int32_t>::value || std::is_same<T, int64_t>::value) return "int";
                    if (std::is_same<T, double>::value) return "float";
                    if (std::is_same<T, bool>::value) return "bool";
                    return "unknown";
                }

                ::value::ValueType elementType() const override
                {
                    if (std::is_same<T, int>::value || std::is_same<T, int32_t>::value || std::is_same<T, int64_t>::value) return ::value::ValueType::INT;
                    if (std::is_same<T, double>::value) return ::value::ValueType::FLOAT;
                    if (std::is_same<T, bool>::value) return ::value::ValueType::BOOL;
                    return ::value::ValueType::VOID;
                }

                ::value::Value get(size_t index) const override
                {
                    if (index >= data_.size())
                    {
                        throw std::out_of_range("Array index out of bounds");
                    }
                    return ::value::Value(data_[index]);
                }

                void set(size_t index, const ::value::Value& value) override
                {
                    if (index >= data_.size())
                    {
                        throw std::out_of_range("Array index out of bounds");
                    }

                    if (::value::holdsT<T>(value))
                    {
                        data_[index] = ::value::getT<T>(value);
                    }
                    else
                    {
                        throw std::runtime_error("Type mismatch in array assignment");
                    }
                }

                void reserve(size_t newCapacity) override
                {
                    data_.reserve(newCapacity);
                }

                void resize(size_t newSize) override
                {
                    data_.resize(newSize);
                }

                void clear() override
                {
                    data_.clear();
                }

                bool supportsSIMD() const override
                {
#ifdef MTYPE_SIMD_ENABLED
                    return data_.size() >= mType::value::simd::SIMDThreshold::MIN_ELEMENTS;
#else
                    return false;
#endif
                }

                size_t simdWidth() const override
                {
#ifdef MTYPE_SIMD_ENABLED
                    if (std::is_same<T, int>::value || std::is_same<T, int32_t>::value || std::is_same<T, int64_t>::value) return
                        mType::value::simd::SIMDWidth::INT32;
                    if (std::is_same<T, double>::value) return mType::value::simd::SIMDWidth::FLOAT;
#endif
                    return 1;
                }

                std::unique_ptr<IArray> clone() const override
                {
                    return std::make_unique<PrimitiveArray<T>>(data_);
                }

                // Direct access for SIMD operations
                // Note: std::vector<bool> doesn't provide .data(), so these are only for int/float
                T* data()
                {
                    static_assert(!std::is_same<T, bool>::value,
                                  "data() not available for bool - use get/set instead");
                    return data_.data();
                }

                const T* data() const
                {
                    static_assert(!std::is_same<T, bool>::value,
                                  "data() not available for bool - use get/set instead");
                    return data_.data();
                }

                // PERFORMANCE OPTIMIZATION: Direct access without variant wrapping
                // These methods bypass Value construction for ~2-3x faster access
                // Use when type is known at compile time

                /**
                 * @brief Fast direct read without bounds checking or Value wrapping
                 * CALLER MUST ENSURE: index < size()
                 * Performance: ~1-2 ns (vs ~4-5 ns for get())
                 */
                inline T getDirect(size_t index) const noexcept
                {
                    return data_[index];
                }

                /**
                 * @brief Fast direct write without bounds checking or variant extraction
                 * CALLER MUST ENSURE: index < size()
                 * Performance: ~1-2 ns (vs ~4-5 ns for set())
                 */
                inline void setDirect(size_t index, T value) noexcept
                {
                    data_[index] = value;
                }

                /**
                 * @brief Unchecked get with Value wrapping (internal use)
                 * Bounds check must be done by caller
                 * Performance: ~2-3 ns (vs ~4-5 ns for get())
                 */
                inline ::value::Value getUnchecked(size_t index) const noexcept
                {
                    return ::value::Value(data_[index]);
                }

                /**
                 * @brief Unchecked set with Value extraction (internal use)
                 * Bounds check must be done by caller
                 * Type check STRONGLY RECOMMENDED or std::get will throw std::bad_variant_access
                 * Performance: ~2-3 ns (vs ~4-5 ns for set())
                 */
                inline void setUnchecked(size_t index, const ::value::Value& value)
                {
                    data_[index] = ::value::getT<T>(value);
                }

            private:
                std::vector<T> data_;

                // Note: We use unaligned SIMD intrinsics (_mm_loadu_*, _mm_storeu_*)
                // std::vector doesn't guarantee SIMD alignment (only alignof(T))
                // For aligned intrinsics, would need custom allocator with std::aligned_alloc()
            };

            // Type aliases for clarity
            // Note: Using 'int64_t' to match the Value variant type (line 45 in ValueType.hpp)
            // Full 64-bit integer support
            using IntArray = PrimitiveArray<int64_t>;
            using FloatArray = PrimitiveArray<double>;
            using BoolArray = PrimitiveArray<bool>;
        } // namespace arrays
    } // namespace value
} // namespace mType
