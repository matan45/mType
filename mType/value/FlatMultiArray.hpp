#pragma once
#include "ValueType.hpp"
#include <vector>
#include <memory>
#include <numeric>
#include <stdexcept>

namespace value
{
    /**
     * @brief Flattened multi-dimensional array implementation for optimal cache performance
     *
     * Instead of nested arrays, stores all elements in a single contiguous buffer
     * and calculates indices using stride arithmetic. This provides:
     * - Better cache locality
     * - Single allocation instead of recursive allocations
     * - Reduced memory overhead
     * - Better performance for large multi-dimensional arrays
     */
    class FlatMultiArray
    {
    private:
        std::vector<Value> data;           // Contiguous storage for all elements
        std::vector<size_t> dimensions;    // Size of each dimension [n1, n2, n3, ...]
        std::vector<size_t> strides;       // Stride for each dimension
        Value defaultValue;                // Default value for initialization

        /**
         * @brief Calculate linear index from multi-dimensional indices
         * @param indices Vector of indices for each dimension
         * @return Linear index into the flat data array
         */
        size_t calculateLinearIndex(const std::vector<size_t>& indices) const {
            if (indices.size() != dimensions.size()) {
                return SIZE_MAX; // Invalid index
            }

            size_t linearIndex = 0;
            for (size_t i = 0; i < indices.size(); ++i) {
                if (indices[i] >= dimensions[i]) {
                    return SIZE_MAX; // Out of bounds
                }
                linearIndex += indices[i] * strides[i];
            }
            return linearIndex;
        }

        /**
         * @brief Calculate strides for each dimension
         * For array[n1][n2][n3], strides are [n2*n3, n3, 1]
         */
        void calculateStrides() {
            strides.resize(dimensions.size());
            if (dimensions.empty()) return;

            strides.back() = 1; // Last dimension has stride 1
            for (int i = static_cast<int>(dimensions.size()) - 2; i >= 0; --i) {
                strides[i] = strides[i + 1] * dimensions[i + 1];
            }
        }

        /**
         * @brief Calculate total number of elements
         */
        size_t calculateTotalSize() const {
            if (dimensions.empty()) return 0;
            return std::accumulate(dimensions.begin(), dimensions.end(),
                                 size_t(1), std::multiplies<size_t>());
        }

    public:
        /**
         * @brief Construct flattened multi-dimensional array
         * @param dims Dimensions for each axis [n1, n2, n3, ...]
         * @param defaultVal Default value to initialize elements
         */
        explicit FlatMultiArray(const std::vector<size_t>& dims, const Value& defaultVal = std::monostate{})
            : dimensions(dims), defaultValue(defaultVal) {

            calculateStrides();
            size_t totalSize = calculateTotalSize();
            data.resize(totalSize, defaultValue);
        }

        /**
         * @brief Get element at multi-dimensional index
         * @param indices Vector of indices [i1, i2, i3, ...]
         * @return Value at the specified position
         */
        Value get(const std::vector<size_t>& indices) const {
            size_t linearIndex = calculateLinearIndex(indices);
            if (linearIndex == SIZE_MAX || linearIndex >= data.size()) {
                return std::monostate{}; // Return null for out of bounds
            }
            return data[linearIndex];
        }

        /**
         * @brief Set element at multi-dimensional index
         * @param indices Vector of indices [i1, i2, i3, ...]
         * @param value Value to set
         */
        void set(const std::vector<size_t>& indices, const Value& value) {
            size_t linearIndex = calculateLinearIndex(indices);
            if (linearIndex == SIZE_MAX) {
                // This indicates invalid indices (out of bounds or wrong number of dimensions)
                throw std::out_of_range("Invalid multi-dimensional array indices");
            }
            if (linearIndex >= data.size()) {
                throw std::out_of_range("Calculated index exceeds array bounds");
            }
            data[linearIndex] = value;
        }

        /**
         * @brief Get element at single index (for 1D arrays)
         * @param index Linear index
         * @return Value at the specified position
         */
        Value get(size_t index) const {
            if (index >= data.size()) {
                throw std::out_of_range("Single-dimensional array index " + std::to_string(index) +
                                      " exceeds array size of " + std::to_string(data.size()) + " elements");
            }
            return data[index];
        }

        /**
         * @brief Set element at single index (for 1D arrays)
         * @param index Linear index
         * @param value Value to set
         */
        void set(size_t index, const Value& value) {
            if (index >= data.size()) {
                throw std::out_of_range("Single-dimensional array assignment index " + std::to_string(index) +
                                      " exceeds array size of " + std::to_string(data.size()) + " elements");
            }
            data[index] = value;
        }

        /**
         * @brief Get sub-array at specified first dimension index
         * Creates a view into the flattened array for multi-dimensional access
         * @param index Index for the first dimension
         * @return Shared pointer to sub-array view
         */
        std::shared_ptr<FlatMultiArray> getSubArray(size_t index) const {
            if (dimensions.empty() || index >= dimensions[0]) {
                return nullptr;
            }

            // Create sub-dimensions (remove first dimension)
            std::vector<size_t> subDims(dimensions.begin() + 1, dimensions.end());
            if (subDims.empty()) {
                // This would be a scalar, return null
                return nullptr;
            }

            auto subArray = std::make_shared<FlatMultiArray>(subDims, defaultValue);

            // Copy relevant portion of data
            size_t subArraySize = subArray->totalSize();
            size_t startOffset = index * strides[0];

            for (size_t i = 0; i < subArraySize; ++i) {
                if (startOffset + i < data.size()) {
                    subArray->data[i] = data[startOffset + i];
                }
            }

            return subArray;
        }

        /**
         * @brief Get total number of elements in the array
         */
        size_t totalSize() const {
            return data.size();
        }

        /**
         * @brief Get dimensions of the array
         */
        const std::vector<size_t>& getDimensions() const {
            return dimensions;
        }

        /**
         * @brief Get number of dimensions
         */
        size_t getRank() const {
            return dimensions.size();
        }

        /**
         * @brief Check if array is empty
         */
        bool empty() const {
            return data.empty();
        }

        /**
         * @brief Get size of first dimension (for compatibility)
         */
        size_t size() const {
            return dimensions.empty() ? 0 : dimensions[0];
        }
    };
}