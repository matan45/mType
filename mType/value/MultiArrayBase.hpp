#pragma once
#include "ValueType.hpp"
#include <vector>
#include <memory>
#include <numeric>
#include <stdexcept>

namespace value
{
    /**
     * @brief Base class for multi-dimensional array implementations
     *
     * Provides common functionality for FlatMultiArray and SparseMultiArray:
     * - Stride-based indexing for multi-dimensional access
     * - View semantics (sub-arrays as views, not copies)
     * - Dimension and size calculations
     * - Common interface methods
     *
     * Design Principles:
     * - Template Method Pattern: Base class provides algorithms, derived classes provide storage
     * - DRY Principle: Eliminates 150+ lines of duplicated code
     * - View Pattern: Sub-arrays are lightweight views into parent data
     */
    template<typename Derived>
    class MultiArrayBase : public std::enable_shared_from_this<Derived>
    {
    protected:
        std::vector<size_t> dimensions_;    // Size of each dimension [n1, n2, n3, ...]
        std::vector<size_t> strides_;       // Stride for each dimension [s1, s2, s3, ...]
        Value defaultValue_;                // Default value for initialization

        // View support: allows sub-arrays to reference parent's data
        std::shared_ptr<Derived> parent_;   // Parent array (nullptr if root)
        size_t viewOffset_;                 // Offset into parent's data

        /**
         * @brief Protected constructor for base class
         */
        MultiArrayBase(const std::vector<size_t>& dims, const Value& defaultVal)
            : dimensions_(dims), defaultValue_(defaultVal), parent_(nullptr), viewOffset_(0)
        {
            calculateStrides();
        }

        /**
         * @brief Protected constructor for view creation
         */
        MultiArrayBase(std::shared_ptr<Derived> parentArray, size_t offset,
                      const std::vector<size_t>& dims, const Value& defaultVal)
            : dimensions_(dims), defaultValue_(defaultVal),
              parent_(parentArray), viewOffset_(offset)
        {
            calculateStrides();
        }

    public:
        virtual ~MultiArrayBase() = default;

        // Common interface methods

        /**
         * @brief Get total number of elements
         */
        size_t totalSize() const {
            return calculateTotalSize();
        }

        /**
         * @brief Get dimensions of the array
         */
        const std::vector<size_t>& getDimensions() const {
            return dimensions_;
        }

        /**
         * @brief Get number of dimensions (rank)
         */
        size_t getRank() const {
            return dimensions_.size();
        }

        /**
         * @brief Check if array is empty
         */
        bool empty() const {
            return dimensions_.empty();
        }

        /**
         * @brief Get size of first dimension
         */
        size_t size() const {
            return dimensions_.empty() ? 0 : dimensions_[0];
        }

        /**
         * @brief Check if array has same dimensions
         */
        bool hasDimensions(const std::vector<size_t>& dims) const {
            return dimensions_ == dims;
        }

    protected:
        // Common helper methods

        /**
         * @brief Check if this is a view into another array
         */
        bool isView() const {
            return parent_ != nullptr;
        }

        /**
         * @brief Get the effective offset for data access
         */
        size_t getEffectiveOffset() const {
            return isView() ? viewOffset_ : 0;
        }

        /**
         * @brief Calculate linear index from multi-dimensional indices
         * @param indices Vector of indices for each dimension
         * @return Linear index, or SIZE_MAX if invalid
         */
        size_t calculateLinearIndex(const std::vector<size_t>& indices) const {
            if (indices.size() != dimensions_.size()) {
                return SIZE_MAX; // Invalid index
            }

            size_t linearIndex = 0;
            for (size_t i = 0; i < indices.size(); ++i) {
                if (indices[i] >= dimensions_[i]) {
                    return SIZE_MAX; // Out of bounds
                }
                linearIndex += indices[i] * strides_[i];
            }
            return linearIndex;
        }

        /**
         * @brief Calculate strides for each dimension
         * For array[n1][n2][n3], strides are [n2*n3, n3, 1]
         */
        void calculateStrides() {
            strides_.resize(dimensions_.size());
            if (dimensions_.empty()) return;

            strides_.back() = 1; // Last dimension has stride 1
            for (int i = static_cast<int>(dimensions_.size()) - 2; i >= 0; --i) {
                strides_[i] = strides_[i + 1] * dimensions_[i + 1];
            }
        }

        /**
         * @brief Calculate total number of elements
         */
        size_t calculateTotalSize() const {
            if (dimensions_.empty()) return 0;
            return std::accumulate(dimensions_.begin(), dimensions_.end(),
                                 size_t(1), std::multiplies<size_t>());
        }

        /**
         * @brief Get sub-array dimensions (removes first dimension)
         */
        std::vector<size_t> getSubDimensions() const {
            if (dimensions_.empty()) return {};
            return std::vector<size_t>(dimensions_.begin() + 1, dimensions_.end());
        }

        /**
         * @brief Calculate offset for sub-array at given index
         */
        size_t calculateSubArrayOffset(size_t index) const {
            if (dimensions_.empty() || index >= dimensions_[0]) {
                return SIZE_MAX;
            }
            return getEffectiveOffset() + (index * strides_[0]);
        }

        /**
         * @brief Get root parent for view creation
         */
        std::shared_ptr<Derived> getRootParent() {
            return isView() ? parent_ : this->shared_from_this();
        }

        std::shared_ptr<Derived> getRootParent() const {
            // Safe to cast away const since view constructors only store the pointer, not modify through it
            return isView() ? parent_ :
                std::const_pointer_cast<Derived>(this->shared_from_this());
        }
    };

} // namespace value
