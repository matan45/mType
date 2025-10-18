#pragma once
#include "ValueType.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <algorithm>

namespace value
{
    /**
     * @brief Adaptive multi-dimensional array with sparse matrix optimization
     *
     * Automatically switches between dense and sparse storage based on:
     * - Sparsity ratio (non-default elements / total elements)
     * - Memory usage thresholds
     * - Array size constraints
     *
     * Supports view semantics: sub-arrays are views into parent data, not copies.
     * Modifications to sub-arrays affect the parent array.
     */
    class SparseMultiArray : public std::enable_shared_from_this<SparseMultiArray>
    {
    public:
        enum class StorageMode
        {
            DENSE, // Full contiguous storage (like current FlatMultiArray)
            SPARSE // Hash map for non-default values only
        };

    private:
        std::vector<Value> denseData; // Dense storage when beneficial (only used if not a view)
        std::unordered_map<size_t, Value> sparseData; // Sparse storage for scattered data (only used if not a view)
        std::vector<size_t> dimensions; // Size of each dimension [n1, n2, n3, ...]
        std::vector<size_t> strides; // Stride for each dimension
        Value defaultValue; // Default value for unset elements

        StorageMode currentMode; // Current storage strategy
        size_t nonDefaultCount; // Count of non-default elements
        size_t totalAccesses; // Total access operations
        size_t sparseAccesses; // Accesses to sparse locations

        // View support: allows sub-arrays to reference parent's data instead of copying
        std::shared_ptr<SparseMultiArray> parent; // Parent array (nullptr if this is not a view)
        size_t viewOffset; // Offset into parent's data (0 if not a view)

        /**
         * @brief Check if this is a view into another array
         */
        bool isView() const {
            return parent != nullptr;
        }

        /**
         * @brief Get reference to the actual dense data storage (own or parent's)
         */
        std::vector<Value>& getDenseDataStorage() {
            return isView() ? parent->getDenseDataStorage() : denseData;
        }

        const std::vector<Value>& getDenseDataStorage() const {
            return isView() ? parent->getDenseDataStorage() : denseData;
        }

        /**
         * @brief Get reference to the actual sparse data storage (own or parent's)
         */
        std::unordered_map<size_t, Value>& getSparseDataStorage() {
            return isView() ? parent->getSparseDataStorage() : sparseData;
        }

        const std::unordered_map<size_t, Value>& getSparseDataStorage() const {
            return isView() ? parent->getSparseDataStorage() : sparseData;
        }

        /**
         * @brief Get the effective offset for data access
         */
        size_t getEffectiveOffset() const {
            return isView() ? viewOffset : 0;
        }

        /**
         * @brief Get the root parent's storage mode
         */
        StorageMode getEffectiveMode() const {
            return isView() ? parent->getEffectiveMode() : currentMode;
        }

        /**
         * @brief Update non-default count (called by child views)
         */
        void updateNonDefaultCount(bool wasDefault, bool isDefault) {
            if (wasDefault && !isDefault) {
                ++nonDefaultCount;
            } else if (!wasDefault && isDefault) {
                --nonDefaultCount;
            }
        }

        // Configuration thresholds
        static constexpr double SPARSE_THRESHOLD = 0.1; // Switch to sparse if <10% filled
        static constexpr double DENSE_THRESHOLD = 0.3; // Switch to dense if >30% filled
        static constexpr size_t MIN_SIZE_FOR_SPARSE = 1000; // Only optimize large arrays
        static constexpr size_t MAX_DENSE_SIZE = 10000000; // 10M elements max for dense

        /**
         * @brief Calculate linear index from multi-dimensional indices
         */
        size_t calculateLinearIndex(const std::vector<size_t>& indices) const
        {
            if (indices.size() != dimensions.size())
            {
                return SIZE_MAX;
            }

            size_t linearIndex = 0;
            for (size_t i = 0; i < indices.size(); ++i)
            {
                if (indices[i] >= dimensions[i])
                {
                    return SIZE_MAX;
                }
                linearIndex += indices[i] * strides[i];
            }
            return linearIndex;
        }

        /**
         * @brief Calculate strides for each dimension
         */
        void calculateStrides()
        {
            strides.resize(dimensions.size());
            if (dimensions.empty()) return;

            strides.back() = 1;
            for (int i = static_cast<int>(dimensions.size()) - 2; i >= 0; --i)
            {
                strides[i] = strides[i + 1] * dimensions[i + 1];
            }
        }

        /**
         * @brief Calculate total number of elements
         */
        size_t calculateTotalSize() const
        {
            if (dimensions.empty()) return 0;
            return std::accumulate(dimensions.begin(), dimensions.end(),
                                   size_t(1), std::multiplies<size_t>());
        }

        /**
         * @brief Determine optimal storage mode based on current state
         */
        StorageMode calculateOptimalMode() const
        {
            size_t totalSize = calculateTotalSize();

            // Always use dense for small arrays
            if (totalSize < MIN_SIZE_FOR_SPARSE)
            {
                return StorageMode::DENSE;
            }

            // Force sparse for very large arrays
            if (totalSize > MAX_DENSE_SIZE)
            {
                return StorageMode::SPARSE;
            }

            // Calculate sparsity ratio
            double sparsityRatio = static_cast<double>(nonDefaultCount) / totalSize;

            // Hysteresis: different thresholds for switching vs staying
            if (currentMode == StorageMode::SPARSE)
            {
                return (sparsityRatio > DENSE_THRESHOLD) ? StorageMode::DENSE : StorageMode::SPARSE;
            }
            else
            {
                return (sparsityRatio < SPARSE_THRESHOLD) ? StorageMode::SPARSE : StorageMode::DENSE;
            }
        }

        /**
         * @brief Convert from dense to sparse storage
         */
        void convertToSparse()
        {
            sparseData.clear();
            sparseData.reserve(nonDefaultCount * 2); // Reserve with some headroom

            for (size_t i = 0; i < denseData.size(); ++i)
            {
                if (!valuesEqual(denseData[i], defaultValue))
                {
                    sparseData[i] = denseData[i];
                }
            }

            denseData.clear();
            denseData.shrink_to_fit();
            currentMode = StorageMode::SPARSE;
        }

        /**
         * @brief Convert from sparse to dense storage
         */
        void convertToDense()
        {
            size_t totalSize = calculateTotalSize();
            denseData.resize(totalSize, defaultValue);

            for (const auto& [index, value] : sparseData)
            {
                if (index < totalSize)
                {
                    denseData[index] = value;
                }
            }

            sparseData.clear();
            currentMode = StorageMode::DENSE;
        }

        /**
         * @brief Check if two values are equal (handles different Value types)
         */
        bool valuesEqual(const Value& a, const Value& b) const
        {
            // Handle the different value types properly
            if (a.index() != b.index()) return false;

            return std::visit([&b](const auto& aVal) -> bool
            {
                return std::visit([&aVal](const auto& bVal) -> bool
                {
                    using AType = std::decay_t<decltype(aVal)>;
                    using BType = std::decay_t<decltype(bVal)>;

                    if constexpr (std::is_same_v<AType, BType>)
                    {
                        return aVal == bVal;
                    }
                    else
                    {
                        return false;
                    }
                }, b);
            }, a);
        }

        /**
         * @brief Adaptive storage mode adjustment
         */
        void maybeAdjustStorageMode()
        {
            // Only check every N operations to avoid overhead
            if (++totalAccesses % 100 != 0) return;

            StorageMode optimalMode = calculateOptimalMode();
            if (optimalMode != currentMode)
            {
                switch (optimalMode)
                {
                case StorageMode::SPARSE:
                    convertToSparse();
                    break;
                case StorageMode::DENSE:
                    convertToDense();
                    break;
                default:
                    break;
                }
            }
        }

    public:
        /**
         * @brief Construct adaptive multi-dimensional array
         */
        explicit SparseMultiArray(const std::vector<size_t>& dims, const Value& defaultVal = std::monostate{})
            : dimensions(dims), defaultValue(defaultVal), nonDefaultCount(0),
              totalAccesses(0), sparseAccesses(0), parent(nullptr), viewOffset(0)
        {
            calculateStrides();
            size_t totalSize = calculateTotalSize();

            // Choose initial storage mode
            if (totalSize < MIN_SIZE_FOR_SPARSE)
            {
                currentMode = StorageMode::DENSE;
                denseData.resize(totalSize, defaultValue);
            }
            else if (totalSize > MAX_DENSE_SIZE)
            {
                currentMode = StorageMode::SPARSE;
                sparseData.reserve(std::min(totalSize / 10, size_t(1000))); // Conservative estimate
            }
            else
            {
                // Start with sparse for large arrays, will adapt based on usage
                currentMode = StorageMode::SPARSE;
                sparseData.reserve(100); // Small initial capacity
            }
        }

        /**
         * @brief Construct a view into parent array (private, used by getSubArray)
         * @param parentArray Parent array to create view into
         * @param offset Offset into parent's data
         * @param dims Dimensions for this view
         * @param defaultVal Default value
         */
    private:
        SparseMultiArray(std::shared_ptr<SparseMultiArray> parentArray, size_t offset,
                        const std::vector<size_t>& dims, const Value& defaultVal)
            : dimensions(dims), defaultValue(defaultVal), parent(parentArray), viewOffset(offset),
              nonDefaultCount(0), totalAccesses(0), sparseAccesses(0)
        {
            calculateStrides();
            // currentMode is inherited from parent via getEffectiveMode()
            currentMode = parent->getEffectiveMode();
            // No data allocation - this is a view into parent
        }

    public:

        /**
         * @brief Get element at multi-dimensional index
         */
        Value get(const std::vector<size_t>& indices) const
        {
            size_t linearIndex = calculateLinearIndex(indices);
            if (linearIndex == SIZE_MAX)
            {
                return std::monostate{};
            }

            size_t effectiveIndex = getEffectiveOffset() + linearIndex;
            StorageMode effectiveMode = getEffectiveMode();

            switch (effectiveMode)
            {
            case StorageMode::DENSE:
                {
                    const auto& denseStorage = getDenseDataStorage();
                    if (effectiveIndex >= denseStorage.size())
                    {
                        return std::monostate{};
                    }
                    return denseStorage[effectiveIndex];
                }

            case StorageMode::SPARSE:
                {
                    const auto& sparseStorage = getSparseDataStorage();
                    auto it = sparseStorage.find(effectiveIndex);
                    return (it != sparseStorage.end()) ? it->second : defaultValue;
                }

            default:
                return defaultValue;
            }
        }

        /**
         * @brief Set element at multi-dimensional index
         */
        void set(const std::vector<size_t>& indices, const Value& value)
        {
            size_t linearIndex = calculateLinearIndex(indices);
            if (linearIndex == SIZE_MAX)
            {
                throw std::out_of_range("Invalid multi-dimensional array indices");
            }

            size_t effectiveIndex = getEffectiveOffset() + linearIndex;
            bool isDefault = valuesEqual(value, defaultValue);
            StorageMode effectiveMode = getEffectiveMode();

            switch (effectiveMode)
            {
            case StorageMode::DENSE:
                {
                    auto& denseStorage = getDenseDataStorage();
                    if (effectiveIndex >= denseStorage.size())
                    {
                        throw std::out_of_range("Index exceeds array bounds");
                    }

                    bool wasDefault = valuesEqual(denseStorage[effectiveIndex], defaultValue);
                    denseStorage[effectiveIndex] = value;

                    // Update non-default count (only for root, not views)
                    if (!isView())
                    {
                        if (wasDefault && !isDefault)
                        {
                            ++nonDefaultCount;
                        }
                        else if (!wasDefault && isDefault)
                        {
                            --nonDefaultCount;
                        }
                    }
                    else
                    {
                        // For views, update parent's count
                        parent->updateNonDefaultCount(wasDefault, isDefault);
                    }
                }
                break;

            case StorageMode::SPARSE:
                {
                    auto& sparseStorage = getSparseDataStorage();
                    auto it = sparseStorage.find(effectiveIndex);
                    bool exists = (it != sparseStorage.end());

                    if (isDefault)
                    {
                        // Setting to default - remove from sparse storage
                        if (exists)
                        {
                            sparseStorage.erase(it);
                            if (!isView())
                            {
                                --nonDefaultCount;
                            }
                            else
                            {
                                parent->updateNonDefaultCount(false, true);
                            }
                        }
                    }
                    else
                    {
                        // Setting to non-default value
                        if (!exists)
                        {
                            sparseStorage[effectiveIndex] = value;
                            if (!isView())
                            {
                                ++nonDefaultCount;
                            }
                            else
                            {
                                parent->updateNonDefaultCount(true, false);
                            }
                        }
                        else
                        {
                            it->second = value;
                        }
                    }
                }
                break;
            }

            // Periodically check if storage mode should be adjusted (only for root)
            if (!isView())
            {
                const_cast<SparseMultiArray*>(this)->maybeAdjustStorageMode();
            }
        }

        /**
         * @brief Get current storage mode (for debugging/monitoring)
         */
        StorageMode getStorageMode() const { return currentMode; }

        /**
         * @brief Get sparsity statistics
         */
        struct SparsityStats
        {
            double sparsityRatio;
            size_t nonDefaultElements;
            size_t totalElements;
            StorageMode currentMode;
            size_t memoryUsed; // Approximate memory usage in bytes
        };

        SparsityStats getSparsityStats() const
        {
            size_t totalSize = calculateTotalSize();
            size_t memoryUsed = 0;

            switch (currentMode)
            {
            case StorageMode::DENSE:
                memoryUsed = denseData.capacity() * sizeof(Value);
                break;
            case StorageMode::SPARSE:
                memoryUsed = sparseData.size() * (sizeof(size_t) + sizeof(Value));
                break;
            }

            return {
                static_cast<double>(nonDefaultCount) / std::max(totalSize, size_t(1)),
                nonDefaultCount,
                totalSize,
                currentMode,
                memoryUsed
            };
        }

        // Existing interface compatibility
        size_t totalSize() const { return calculateTotalSize(); }
        const std::vector<size_t>& getDimensions() const { return dimensions; }
        size_t getRank() const { return dimensions.size(); }
        bool empty() const { return dimensions.empty(); }
        size_t size() const { return dimensions.empty() ? 0 : dimensions[0]; }

        /**
         * @brief Reset array with new default value (for pool reuse)
         * Note: Only works on root arrays, not views
         */
        void reset(const Value& newDefaultValue = std::monostate{})
        {
            if (isView())
            {
                // Cannot reset a view - must reset the root parent
                throw std::logic_error("Cannot reset a view array");
            }

            defaultValue = newDefaultValue;
            nonDefaultCount = 0;
            totalAccesses = 0;
            sparseAccesses = 0;

            switch (currentMode)
            {
            case StorageMode::DENSE:
                std::fill(denseData.begin(), denseData.end(), defaultValue);
                break;
            case StorageMode::SPARSE:
                sparseData.clear();
                break;
            }
        }

        /**
         * @brief Check if array has same dimensions (for pool compatibility)
         */
        bool hasDimensions(const std::vector<size_t>& dims) const
        {
            return dimensions == dims;
        }

        /**
         * @brief Get sub-array for chained indexing compatibility (e.g., arr[0][1])
         * Creates a VIEW into the parent array - modifications to the sub-array affect the parent!
         * @param index The first dimension index
         * @return Shared pointer to sub-array view
         */
        std::shared_ptr<SparseMultiArray> getSubArray(size_t index) const
        {
            if (dimensions.empty() || index >= dimensions[0])
            {
                return nullptr;
            }

            // Create sub-dimensions (remove first dimension)
            std::vector<size_t> subDimensions(dimensions.begin() + 1, dimensions.end());

            if (subDimensions.empty())
            {
                // If this would result in 0D array, return null
                return nullptr;
            }

            // Calculate offset into parent's data for this sub-array
            size_t subArrayOffset = getEffectiveOffset() + (index * strides[0]);

            // Get the root parent (if this is already a view, use the same root parent)
            std::shared_ptr<SparseMultiArray> rootParent = isView() ? parent :
                std::const_pointer_cast<SparseMultiArray>(shared_from_this());

            // Create a VIEW into the parent array (not a copy!)
            // This allows modifications to propagate to the parent
            auto subArray = std::shared_ptr<SparseMultiArray>(
                new SparseMultiArray(rootParent, subArrayOffset, subDimensions, defaultValue)
            );

            return subArray;
        }
    };
}
