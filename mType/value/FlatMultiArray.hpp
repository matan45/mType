#pragma once
#include "MultiArrayBase.hpp"
#include <vector>
#include <memory>
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
    class FlatMultiArray : public MultiArrayBase<FlatMultiArray>
    {
    private:
        std::vector<Value> data_;  // Contiguous storage for all elements (only used if not a view)

        /**
         * @brief Get reference to the actual data storage (own or parent's)
         */
        std::vector<Value>& getDataStorage() {
            return isView() ? parent_->getDataStorage() : data_;
        }

        const std::vector<Value>& getDataStorage() const {
            return isView() ? parent_->getDataStorage() : data_;
        }

    public:
        /**
         * @brief Construct flattened multi-dimensional array
         * @param dims Dimensions for each axis [n1, n2, n3, ...]
         * @param defaultVal Default value to initialize elements
         */
        explicit FlatMultiArray(const std::vector<size_t>& dims, const Value& defaultVal = std::monostate{})
            : MultiArrayBase<FlatMultiArray>(dims, defaultVal) {
            size_t totalSize = calculateTotalSize();
            data_.resize(totalSize, defaultValue_);
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
            : MultiArrayBase<FlatMultiArray>(parentArray, offset, dims, defaultVal) {
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
            auto subDims = getSubDimensions();
            if (subDims.empty()) return nullptr;

            size_t subArrayOffset = calculateSubArrayOffset(index);
            if (subArrayOffset == SIZE_MAX) return nullptr;

            auto rootParent = getRootParent();
            return std::shared_ptr<FlatMultiArray>(
                new FlatMultiArray(rootParent, subArrayOffset, subDims, defaultValue_)
            );
        }

        // Common interface inherited from MultiArrayBase:
        // - totalSize()
        // - getDimensions()
        // - getRank()
        // - empty()
        // - size()
        // - hasDimensions()

        /**
         * @brief Reset array with new default value (for pool reuse)
         * @param newDefaultValue New value to fill the array with
         * Note: Only works on root arrays, not views
         */
        void reset(const Value& newDefaultValue = std::monostate{}) {
            if (isView()) {
                throw std::logic_error("Cannot reset a view array");
            }
            defaultValue_ = newDefaultValue;
            std::fill(data_.begin(), data_.end(), defaultValue_);
        }
    };
}