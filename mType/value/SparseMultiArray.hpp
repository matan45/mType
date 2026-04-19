#pragma once
#include "MultiArrayBase.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
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
     * VIEW SEMANTICS - MEMORY SAFETY CRITICAL:
     * ==========================================
     *
     * Sub-arrays are VIEWS (aliases) into parent data, NOT independent copies!
     *
     * Example demonstrating view behavior:
     * ```cpp
     * auto arr = std::make_shared<SparseMultiArray>(std::vector<size_t>{1000, 1000}, 0);
     * arr->set({0, 0}, 42);
     *
     * auto subArr = arr->getSubArray(0);  // Creates VIEW, not copy
     * subArr->set({0}, 99);               // Modifies arr[0][0]!
     *
     * arr->get({0, 0});  // Returns 99 (changed by view!)
     * ```
     *
     * LIFETIME & OWNERSHIP:
     * - Root array owns denseData_ or sparseData_
     * - Views hold shared_ptr to root, keeping it alive
     * - Destroying root's external references is SAFE - views keep data alive
     * - All sibling views see each other's modifications immediately
     *
     * THREAD SAFETY:
     * - Concurrent reads: SAFE (if no writes and no mode transitions)
     * - Concurrent writes or read+write: UNDEFINED BEHAVIOR
     * - Mode transitions (sparse↔dense): NOT thread-safe
     * - Requires external synchronization for concurrent modification
     */
    class SparseMultiArray : public MultiArrayBase<SparseMultiArray>
    {
    public:
        enum class StorageMode
        {
            DENSE, // Full contiguous storage (like current FlatMultiArray)
            SPARSE // Hash map for non-default values only
        };

    private:
        std::vector<Value> denseData_; // Dense storage when beneficial (only used if not a view)
        std::unordered_map<size_t, Value> sparseData_; // Sparse storage for scattered data (only used if not a view)

        StorageMode currentMode_; // Current storage strategy
        size_t nonDefaultCount_; // Count of non-default elements
        size_t totalAccesses_; // Total access operations
        size_t sparseAccesses_; // Accesses to sparse locations

        /**
         * @brief Get reference to the actual dense data storage (own or parent's)
         */
        std::vector<Value>& getDenseDataStorage() {
            return isView() ? parent_->getDenseDataStorage() : denseData_;
        }

        const std::vector<Value>& getDenseDataStorage() const {
            return isView() ? parent_->getDenseDataStorage() : denseData_;
        }

        /**
         * @brief Get reference to the actual sparse data storage (own or parent's)
         */
        std::unordered_map<size_t, Value>& getSparseDataStorage() {
            return isView() ? parent_->getSparseDataStorage() : sparseData_;
        }

        const std::unordered_map<size_t, Value>& getSparseDataStorage() const {
            return isView() ? parent_->getSparseDataStorage() : sparseData_;
        }

        /**
         * @brief Get the root parent's storage mode
         */
        StorageMode getEffectiveMode() const {
            return isView() ? parent_->getEffectiveMode() : currentMode_;
        }

        /**
         * @brief Update non-default count (called by child views)
         */
        void updateNonDefaultCount(bool wasDefault, bool isDefault) {
            if (wasDefault && !isDefault) {
                ++nonDefaultCount_;
            } else if (!wasDefault && isDefault) {
                --nonDefaultCount_;
            }
        }

        // Configuration thresholds
        static constexpr double SPARSE_THRESHOLD = 0.1; // Switch to sparse if <10% filled
        static constexpr double DENSE_THRESHOLD = 0.3; // Switch to dense if >30% filled
        static constexpr size_t MIN_SIZE_FOR_SPARSE = 1000; // Only optimize large arrays
        static constexpr size_t MAX_DENSE_SIZE = 10000000; // 10M elements max for dense

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
            double sparsityRatio = static_cast<double>(nonDefaultCount_) / totalSize;

            // Hysteresis: different thresholds for switching vs staying
            if (currentMode_ == StorageMode::SPARSE)
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
            sparseData_.clear();
            sparseData_.reserve(nonDefaultCount_ * 2); // Reserve with some headroom

            for (size_t i = 0; i < denseData_.size(); ++i)
            {
                if (!valuesEqual(denseData_[i], defaultValue_))
                {
                    sparseData_[i] = denseData_[i];
                }
            }

            denseData_.clear();
            denseData_.shrink_to_fit();
            currentMode_ = StorageMode::SPARSE;
        }

        /**
         * @brief Convert from sparse to dense storage
         */
        void convertToDense()
        {
            size_t totalSize = calculateTotalSize();
            denseData_.resize(totalSize, defaultValue_);

            for (const auto& [index, value] : sparseData_)
            {
                if (index < totalSize)
                {
                    denseData_[index] = value;
                }
            }

            sparseData_.clear();
            currentMode_ = StorageMode::DENSE;
        }

        /**
         * @brief Check if two values are equal (handles different Value types)
         */
        bool valuesEqual(const Value& a, const Value& b) const
        {
            // Value::operator== is tag+content dispatched (see ValueType.hpp),
            // so this delegates to the standard equality semantics.
            return a == b;
        }

        /**
         * @brief Adaptive storage mode adjustment
         */
        void maybeAdjustStorageMode()
        {
            // Only check every N operations to avoid overhead
            if (++totalAccesses_ % 100 != 0) return;

            StorageMode optimalMode = calculateOptimalMode();
            if (optimalMode != currentMode_)
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
            : MultiArrayBase<SparseMultiArray>(dims, defaultVal),
              nonDefaultCount_(0), totalAccesses_(0), sparseAccesses_(0)
        {
            size_t totalSize = calculateTotalSize();

            // Choose initial storage mode
            if (totalSize < MIN_SIZE_FOR_SPARSE)
            {
                currentMode_ = StorageMode::DENSE;
                denseData_.resize(totalSize, defaultValue_);
            }
            else if (totalSize > MAX_DENSE_SIZE)
            {
                currentMode_ = StorageMode::SPARSE;
                sparseData_.reserve(std::min(totalSize / 10, size_t(1000))); // Conservative estimate
            }
            else
            {
                // Start with sparse for large arrays, will adapt based on usage
                currentMode_ = StorageMode::SPARSE;
                sparseData_.reserve(100); // Small initial capacity
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
            : MultiArrayBase<SparseMultiArray>(parentArray, offset, dims, defaultVal),
              nonDefaultCount_(0), totalAccesses_(0), sparseAccesses_(0)
        {
            // currentMode is inherited from parent via getEffectiveMode()
            currentMode_ = parent_->getEffectiveMode();
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
                    return (it != sparseStorage.end()) ? it->second : defaultValue_;
                }

            default:
                return defaultValue_;
            }
        }

        /**
         * @brief Get element at single-dimension index (convenience method)
         * @param index Index in the first dimension
         * @return Value at the specified index
         */
        Value get(size_t index) const
        {
            // For 1D arrays or first dimension access
            if (dimensions_.size() == 1)
            {
                return get(std::vector<size_t>{index});
            }

            // For multi-dimensional arrays, get returns the element at [index]
            // which requires constructing a vector with just the first index
            size_t linearIndex = index * strides_[0];
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
                    return (it != sparseStorage.end()) ? it->second : defaultValue_;
                }

            default:
                return defaultValue_;
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
            bool isDefault = valuesEqual(value, defaultValue_);
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

                    bool wasDefault = valuesEqual(denseStorage[effectiveIndex], defaultValue_);
                    denseStorage[effectiveIndex] = value;

                    // Update non-default count (only for root, not views)
                    if (!isView())
                    {
                        if (wasDefault && !isDefault)
                        {
                            ++nonDefaultCount_;
                        }
                        else if (!wasDefault && isDefault)
                        {
                            --nonDefaultCount_;
                        }
                    }
                    else
                    {
                        // For views, update parent's count
                        parent_->updateNonDefaultCount(wasDefault, isDefault);
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
                                --nonDefaultCount_;
                            }
                            else
                            {
                                parent_->updateNonDefaultCount(false, true);
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
                                ++nonDefaultCount_;
                            }
                            else
                            {
                                parent_->updateNonDefaultCount(true, false);
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
        StorageMode getStorageMode() const { return currentMode_; }

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

            switch (currentMode_)
            {
            case StorageMode::DENSE:
                memoryUsed = denseData_.capacity() * sizeof(Value);
                break;
            case StorageMode::SPARSE:
                memoryUsed = sparseData_.size() * (sizeof(size_t) + sizeof(Value));
                break;
            }

            return {
                static_cast<double>(nonDefaultCount_) / std::max(totalSize, size_t(1)),
                nonDefaultCount_,
                totalSize,
                currentMode_,
                memoryUsed
            };
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
         * Note: Only works on root arrays, not views
         */
        void reset(const Value& newDefaultValue = std::monostate{})
        {
            if (isView())
            {
                throw std::logic_error("Cannot reset a view array");
            }

            defaultValue_ = newDefaultValue;
            nonDefaultCount_ = 0;
            totalAccesses_ = 0;
            sparseAccesses_ = 0;

            switch (currentMode_)
            {
            case StorageMode::DENSE:
                std::fill(denseData_.begin(), denseData_.end(), defaultValue_);
                break;
            case StorageMode::SPARSE:
                sparseData_.clear();
                break;
            }
        }

        /**
         * @brief Get sub-array for chained indexing compatibility (e.g., arr[0][1])
         * Creates a VIEW into the parent array - modifications to the sub-array affect the parent!
         *
         * Note: This is intentionally non-const because it creates a modifiable view that can
         * alter the parent array's data, making it logically a non-const operation.
         *
         * @param index The first dimension index
         * @return Shared pointer to sub-array view
         */
        std::shared_ptr<SparseMultiArray> getSubArray(size_t index)
        {
            auto subDims = getSubDimensions();
            if (subDims.empty()) return nullptr;

            size_t subArrayOffset = calculateSubArrayOffset(index);
            if (subArrayOffset == SIZE_MAX) return nullptr;

            auto rootParent = getRootParent();
            return std::shared_ptr<SparseMultiArray>(
                new SparseMultiArray(rootParent, subArrayOffset, subDims, defaultValue_)
            );
        }

        // IMultiDimensionalArray interface implementation
        const char* getTypeName() const override {
            return "SparseMultiArray";
        }

        std::shared_ptr<IMultiDimensionalArray> getSubArray(size_t index) const override {
            // Cast away const for view creation (view modifications are tracked separately)
            auto* self = const_cast<SparseMultiArray*>(this);
            return std::static_pointer_cast<IMultiDimensionalArray>(self->getSubArray(index));
        }
    };
}
