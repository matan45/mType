#pragma once
#include "IMultiDimensionalArray.hpp"
#include <cstdint>
#include "ValueType.hpp"
#include "../constants/SecurityConstants.hpp"
#include <vector>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <climits>

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
     *
     * MEMORY SAFETY & LIFETIME GUARANTEES:
     * =====================================
     *
     * 1. OWNERSHIP MODEL:
     *    - Root arrays OWN their data storage (denseData_, sparseData_, etc.)
     *    - View arrays REFERENCE parent's data through shared_ptr
     *    - Parent array is kept alive by shared_ptr as long as ANY view exists
     *    - Views form a reference chain: view -> parent -> root (all kept alive)
     *
     * 2. LIFETIME GUARANTEES:
     *    - Root array lives until all references (including views) are destroyed
     *    - View destruction does NOT invalidate other views of same parent
     *    - Destroying parent's last external reference is SAFE - views keep it alive
     *    - shared_ptr reference counting ensures no dangling pointers
     *
     * 3. REFERENCE SEMANTICS (NOT VALUE SEMANTICS):
     *    - Views are ALIASES to parent data, NOT independent copies
     *    - Modifications through a view AFFECT the parent and all sibling views
     *    - Multiple views of same region see each other's changes immediately
     *    - Example: arr[0][1] = 5 modifies the root array
     *
     * 4. MEMORY CHARACTERISTICS:
     *    - View creation: O(1) time, ~48 bytes memory (no data copy)
     *    - Root array: O(n) memory where n = total elements
     *    - View destruction: O(1) time, releases shared_ptr reference
     *
     * 5. THREAD SAFETY:
     *    - NOT thread-safe by default
     *    - Concurrent reads from multiple threads: SAFE (if no writes)
     *    - Concurrent writes or read+write: UNDEFINED BEHAVIOR (requires external sync)
     *    - View creation from same parent: SAFE (independent shared_ptr copies)
     *
     * 6. COMMON PITFALLS:
     *    - ❌ Assuming views are independent copies (they're aliases!)
     *    - ❌ Ignoring that view modifications affect parent
     *    - ❌ Expecting value semantics (use explicit copy if needed)
     *    - ✅ Understanding shared ownership through shared_ptr
     */
    template<typename Derived>
    class MultiArrayBase : public IMultiDimensionalArray, public std::enable_shared_from_this<Derived>
    {
    protected:
        std::vector<size_t> dimensions_;    // Size of each dimension [n1, n2, n3, ...]
        std::vector<size_t> strides_;       // Stride for each dimension [s1, s2, s3, ...]
        Value defaultValue_;                // Default value for initialization

        // View support: allows sub-arrays to reference parent's data
        // MEMORY SAFETY: parent_ is shared_ptr, ensuring parent stays alive while views exist
        std::shared_ptr<Derived> parent_;   // Parent array (nullptr if root, shared_ptr keeps parent alive)
        size_t viewOffset_;                 // Offset into parent's data (0 if root)

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
         *
         * MEMORY SAFETY CONTRACT:
         * - Takes shared_ptr to parent, ensuring parent stays alive
         * - View does NOT copy data, only stores reference + offset
         * - Parent's data must remain valid for view's lifetime (guaranteed by shared_ptr)
         * - View modifications propagate to parent immediately
         *
         * @param parentArray Shared pointer to parent array (keeps parent alive)
         * @param offset Byte offset into parent's data storage
         * @param dims Dimensions for this view (subset of parent dimensions)
         * @param defaultVal Default value (inherited from parent)
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
         *
         * OWNERSHIP SEMANTICS:
         * - Returns true if this array references parent's data
         * - Returns false if this array owns its data storage
         * - Views always have parent_ != nullptr
         * - Root arrays always have parent_ == nullptr
         */
        bool isView() const {
            return parent_ != nullptr;
        }

        /**
         * @brief Get the effective offset for data access
         *
         * MEMORY LAYOUT:
         * - Root arrays: offset = 0 (access from beginning of owned storage)
         * - View arrays: offset = viewOffset_ (access from middle of parent's storage)
         * - All data access must add this offset to calculated indices
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
         *
         * SECURITY: each multiplication is overflow-checked. A crafted set of
         * dimensions (e.g. {SIZE_MAX/2, 4}) would otherwise silently wrap and
         * produce strides that bypass downstream bounds checks.
         */
        void calculateStrides() {
            strides_.resize(dimensions_.size());
            if (dimensions_.empty()) return;

            strides_.back() = 1; // Last dimension has stride 1
            for (int i = static_cast<int>(dimensions_.size()) - 2; i >= 0; --i) {
                size_t next = dimensions_[i + 1];
                if (next != 0 && strides_[i + 1] > SIZE_MAX / next) {
                    throw std::runtime_error("Array stride overflow");
                }
                strides_[i] = strides_[i + 1] * next;
            }
        }

        /**
         * @brief Calculate total number of elements
         *
         * SECURITY: uses checked multiplication and a hard cap on the total
         * element count to prevent integer overflow when computing array
         * allocations from untrusted dimensions.
         */
        size_t calculateTotalSize() const {
            if (dimensions_.empty()) return 0;
            size_t total = 1;
            for (size_t d : dimensions_) {
                if (d == 0) {
                    return 0;
                }
                if (total > SIZE_MAX / d) {
                    throw std::runtime_error("Array dimension overflow");
                }
                total *= d;
            }
            if (total > constants::security::MAX_ARRAY_DIMENSION_PRODUCT) {
                throw std::runtime_error("Array exceeds maximum element count");
            }
            return total;
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
         *
         * OWNERSHIP CHAIN:
         * - For root arrays: returns this (the data owner)
         * - For view arrays: returns parent (which may be root or another view)
         * - Creates reference chain: new_view -> this -> parent -> ... -> root
         * - All views ultimately reference the same root array's data
         *
         * MEMORY SAFETY:
         * - Returns shared_ptr to prevent parent deallocation
         * - New views hold reference to root, keeping entire chain alive
         * - Even if intermediate views are destroyed, root stays alive
         *
         * CONST CORRECTNESS:
         * - Intentionally NON-CONST because creating a modifiable view is logically
         *   a non-const operation (view can modify parent's data)
         * - Prevents creating mutable views from const arrays (enforces immutability)
         *
         * @return Shared pointer to root array (data owner)
         */
        std::shared_ptr<Derived> getRootParent() {
            return isView() ? parent_ : this->shared_from_this();
        }
    };

} // namespace value
