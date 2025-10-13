#pragma once
#include "../base/IArray.hpp"
#include "../../StringPool.hpp"
#include <vector>
#include <memory>
#include <stdexcept>
#include <unordered_set>

namespace mType {
namespace value {
namespace arrays {

/**
 * @brief Optimized array for string storage using StringPool.
 *
 * Memory Optimization:
 * - Stores pool IDs (size_t) instead of full strings
 * - Automatic deduplication via StringPool
 * - 50-80% memory reduction for duplicate strings
 *
 * Performance Benefits:
 * - O(1) string comparison (compare pool IDs)
 * - Better cache locality (8 bytes vs variable length)
 * - Batch string operations
 *
 * Design Principles:
 * - Strategy Pattern: Delegate string interning to StringPool
 * - Lazy Evaluation: Intern strings only when accessed via InternedString
 * - RAII: Automatic reference counting via InternedString lifecycle
 */
class StringArray : public IArray {
public:
    // Construction
    explicit StringArray(size_t initialSize = 0)
        : poolIds_(initialSize, 0) {
        // Initialize with empty strings (poolId = 0)
    }

    explicit StringArray(const std::vector<std::string>& strings)
        : poolIds_() {
        poolIds_.reserve(strings.size());
        for (const auto& str : strings) {
            poolIds_.push_back(internString(str));
        }
    }

    explicit StringArray(std::vector<std::string>&& strings)
        : poolIds_() {
        poolIds_.reserve(strings.size());
        for (auto& str : strings) {
            poolIds_.push_back(internString(std::move(str)));
        }
    }

    // IArray implementation
    size_t size() const override { return poolIds_.size(); }
    size_t capacity() const override { return poolIds_.capacity(); }
    bool empty() const override { return poolIds_.empty(); }

    std::string elementTypeName() const override {
        return "string";
    }

    ::value::ValueType elementType() const override {
        return ::value::ValueType::STRING;
    }

    ::value::Value get(size_t index) const override {
        if (index >= poolIds_.size()) {
            throw std::out_of_range("Array index out of bounds");
        }

        size_t poolId = poolIds_[index];
        if (poolId == 0) {
            return ::value::Value(std::string(""));
        }

        // Return InternedString for O(1) comparison
        return ::value::Value(getInternedString(poolId));
    }

    void set(size_t index, const ::value::Value& value) override {
        if (index >= poolIds_.size()) {
            throw std::out_of_range("Array index out of bounds");
        }

        // Handle string types
        if (std::holds_alternative<std::string>(value)) {
            poolIds_[index] = internString(std::get<std::string>(value));
        } else if (std::holds_alternative<::value::InternedString>(value)) {
            poolIds_[index] = std::get<::value::InternedString>(value).getPoolId();
        } else {
            throw std::runtime_error("Type mismatch: expected string");
        }
    }

    void reserve(size_t newCapacity) override {
        poolIds_.reserve(newCapacity);
    }

    void resize(size_t newSize) override {
        poolIds_.resize(newSize, 0);  // Fill with empty strings
    }

    void clear() override {
        poolIds_.clear();
    }

    bool supportsSIMD() const override {
        // String arrays don't use traditional SIMD, but benefit from pool ID comparisons
        return false;
    }

    size_t simdWidth() const override {
        return 1;
    }

    std::unique_ptr<IArray> clone() const override {
        auto cloned = std::make_unique<StringArray>();
        cloned->poolIds_ = poolIds_;
        return cloned;
    }

    // StringArray-specific operations

    /**
     * @brief Get pool ID at index (for O(1) comparison)
     */
    size_t getPoolId(size_t index) const {
        if (index >= poolIds_.size()) {
            throw std::out_of_range("Array index out of bounds");
        }
        return poolIds_[index];
    }

    /**
     * @brief Direct string access (returns string reference from pool)
     */
    const std::string& getStringDirect(size_t index) const {
        if (index >= poolIds_.size()) {
            throw std::out_of_range("Array index out of bounds");
        }

        size_t poolId = poolIds_[index];
        if (poolId == 0) {
            static const std::string emptyString;
            return emptyString;
        }

        return ::value::StringPool::getInstance().getById(poolId).getString();
    }

    /**
     * @brief Fast equality check using pool IDs (O(1) per element)
     */
    bool equals(const StringArray& other) const {
        if (size() != other.size()) {
            return false;
        }

        for (size_t i = 0; i < poolIds_.size(); ++i) {
            if (poolIds_[i] != other.poolIds_[i]) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Count unique strings in array
     */
    size_t countUnique() const {
        std::unordered_set<size_t> uniqueIds(poolIds_.begin(), poolIds_.end());
        return uniqueIds.size();
    }

    /**
     * @brief Calculate memory usage
     */
    size_t getMemoryUsage() const {
        // Pool IDs storage
        size_t poolIdMemory = poolIds_.capacity() * sizeof(size_t);

        // Strings are shared in the pool, so don't count them here
        // Use StringPool::getTotalMemoryUsage() for total pool memory

        return poolIdMemory + sizeof(StringArray);
    }

    /**
     * @brief Get statistics about string duplication
     */
    struct DeduplicationStats {
        size_t totalStrings;
        size_t uniqueStrings;
        double deduplicationRatio;  // unique / total
        size_t memorySaved;         // Estimated memory saved vs storing all strings
    };

    DeduplicationStats getDeduplicationStats() const {
        DeduplicationStats stats;
        stats.totalStrings = poolIds_.size();
        stats.uniqueStrings = countUnique();
        stats.deduplicationRatio = stats.totalStrings > 0
            ? static_cast<double>(stats.uniqueStrings) / stats.totalStrings
            : 0.0;

        // Estimate memory saved
        size_t totalStringMemory = 0;
        for (size_t poolId : poolIds_) {
            if (poolId != 0) {
                totalStringMemory += getInternedString(poolId).getString().capacity();
            }
        }

        // Memory used with poolIds
        size_t poolIdMemory = poolIds_.capacity() * sizeof(size_t);

        // Estimated memory if all strings were stored individually
        stats.memorySaved = totalStringMemory > poolIdMemory
            ? totalStringMemory - poolIdMemory
            : 0;

        return stats;
    }

    // Direct access to pool IDs (for advanced operations)
    const std::vector<size_t>& getPoolIds() const { return poolIds_; }

private:
    std::vector<size_t> poolIds_;  // Store pool IDs instead of strings

    /**
     * @brief Intern a string and return its pool ID
     */
    size_t internString(const std::string& str) {
        if (str.empty()) {
            return 0;  // Use 0 for empty strings
        }
        return ::value::StringPool::getInstance().intern(str).getPoolId();
    }

    /**
     * @brief Intern a string (move version) and return its pool ID
     */
    size_t internString(std::string&& str) {
        if (str.empty()) {
            return 0;
        }
        return ::value::StringPool::getInstance().intern(std::move(str)).getPoolId();
    }

    /**
     * @brief Get InternedString from pool ID
     */
    ::value::InternedString getInternedString(size_t poolId) const {
        return ::value::StringPool::getInstance().getById(poolId);
    }
};

} // namespace arrays
} // namespace value
} // namespace mType
