#pragma once
#include "ValueType.hpp"
#include <vector>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <algorithm>

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
     *
     * Supports view semantics: sub-arrays are views into parent data, not copies.
     * Modifications to sub-arrays affect the parent array.
     */
    class FlatMultiArray : public std::enable_shared_from_this<FlatMultiArray>
    {
    private:
        std::vector<Value> data;           // Contiguous storage for all elements (only used if not a view)
        std::vector<size_t> dimensions;    // Size of each dimension [n1, n2, n3, ...]
        std::vector<size_t> strides;       // Stride for each dimension
        Value defaultValue;                // Default value for initialization

        // View support: allows sub-arrays to reference parent's data instead of copying
        std::shared_ptr<FlatMultiArray> parent;  // Parent array (nullptr if this is not a view)
        size_t viewOffset;                       // Offset into parent's data (0 if not a view)

        /**
         * @brief Check if this is a view into another array
         */
        bool isView() const {
            return parent != nullptr;
        }

        /**
         * @brief Get reference to the actual data storage (own or parent's)
         */
        std::vector<Value>& getDataStorage() {
            return isView() ? parent->getDataStorage() : data;
        }

        const std::vector<Value>& getDataStorage() const {
            return isView() ? parent->getDataStorage() : data;
        }

        /**
         * @brief Get the effective offset for data access
         */
        size_t getEffectiveOffset() const {
            return isView() ? viewOffset : 0;
        }

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
            : dimensions(dims), defaultValue(defaultVal), parent(nullptr), viewOffset(0) {

            calculateStrides();
            size_t totalSize = calculateTotalSize();
            data.resize(totalSize, defaultValue);
        }

        /**
         * @brief Construct a view into parent array (private, used by getSubArray)
         * @param parentArray Parent array to create view into
         * @param offset Offset into parent's data
         * @param dims Dimensions for this view
         * @param defaultVal Default value
         */
    private:
        FlatMultiArray(std::shared_ptr<FlatMultiArray> parentArray, size_t offset,
                      const std::vector<size_t>& dims, const Value& defaultVal)
            : dimensions(dims), defaultValue(defaultVal), parent(parentArray), viewOffset(offset) {
            calculateStrides();
            // No data allocation - this is a view into parent
        }

    public:

        /**
         * @brief Get element at multi-dimensional index
         * @param indices Vector of indices [i1, i2, i3, ...]
         * @return Value at the specified position
         */
        Value get(const std::vector<size_t>& indices) const {
            size_t linearIndex = calculateLinearIndex(indices);
            if (linearIndex == SIZE_MAX) {
                return std::monostate{}; // Return null for invalid indices
            }

            const auto& dataStorage = getDataStorage();
            size_t effectiveIndex = getEffectiveOffset() + linearIndex;

            if (effectiveIndex >= dataStorage.size()) {
                return std::monostate{}; // Return null for out of bounds
            }
            return dataStorage[effectiveIndex];
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

            auto& dataStorage = getDataStorage();
            size_t effectiveIndex = getEffectiveOffset() + linearIndex;

            if (effectiveIndex >= dataStorage.size()) {
                throw std::out_of_range("Calculated index exceeds array bounds");
            }
            dataStorage[effectiveIndex] = value;
        }

        /**
         * @brief Get element at single index (for 1D arrays or linear access)
         * @param index Linear index
         * @return Value at the specified position
         */
        Value get(size_t index) const {
            const auto& dataStorage = getDataStorage();
            size_t effectiveIndex = getEffectiveOffset() + index;

            if (index >= calculateTotalSize()) {
                throw std::out_of_range("Single-dimensional array index " + std::to_string(index) +
                                      " exceeds array size of " + std::to_string(calculateTotalSize()) + " elements");
            }
            if (effectiveIndex >= dataStorage.size()) {
                throw std::out_of_range("Effective index exceeds storage bounds");
            }
            return dataStorage[effectiveIndex];
        }

        /**
         * @brief Set element at single index (for 1D arrays or linear access)
         * @param index Linear index
         * @param value Value to set
         */
        void set(size_t index, const Value& value) {
            auto& dataStorage = getDataStorage();
            size_t effectiveIndex = getEffectiveOffset() + index;

            if (index >= calculateTotalSize()) {
                throw std::out_of_range("Single-dimensional array assignment index " + std::to_string(index) +
                                      " exceeds array size of " + std::to_string(calculateTotalSize()) + " elements");
            }
            if (effectiveIndex >= dataStorage.size()) {
                throw std::out_of_range("Effective index exceeds storage bounds");
            }
            dataStorage[effectiveIndex] = value;
        }

        /**
         * @brief Get sub-array at specified first dimension index
         * Creates a VIEW into the parent array - modifications to the sub-array affect the parent!
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

            // Calculate offset into parent's data for this sub-array
            size_t subArrayOffset = getEffectiveOffset() + (index * strides[0]);

            // Get the root parent (if this is already a view, use the same root parent)
            std::shared_ptr<FlatMultiArray> rootParent = isView() ? parent :
                std::const_pointer_cast<FlatMultiArray>(shared_from_this());

            // Create a VIEW into the parent array (not a copy!)
            // This allows modifications to propagate to the parent
            auto subArray = std::shared_ptr<FlatMultiArray>(
                new FlatMultiArray(rootParent, subArrayOffset, subDims, defaultValue)
            );

            return subArray;
        }

        /**
         * @brief Get total number of elements in this array (or view)
         */
        size_t totalSize() const {
            return calculateTotalSize();
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

        /**
         * @brief Reset array with new default value (for pool reuse)
         * @param newDefaultValue New value to fill the array with
         * Note: Only works on root arrays, not views
         */
        void reset(const Value& newDefaultValue = std::monostate{}) {
            if (isView()) {
                // Cannot reset a view - must reset the root parent
                throw std::logic_error("Cannot reset a view array");
            }
            defaultValue = newDefaultValue;
            std::fill(data.begin(), data.end(), defaultValue);
        }

        /**
         * @brief Check if array has same dimensions (for pool compatibility)
         * @param dims Dimensions to compare against
         * @return true if dimensions match exactly
         */
        bool hasDimensions(const std::vector<size_t>& dims) const {
            return dimensions == dims;
        }
    };
}