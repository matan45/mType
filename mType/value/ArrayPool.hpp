#pragma once
#include "FlatMultiArray.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <string>

namespace value
{
    /**
     * @brief Pool configuration and statistics
     */
    struct PoolStats
    {
        size_t totalAllocations = 0;      // Total array requests
        size_t poolHits = 0;              // Arrays served from pool
        size_t poolMisses = 0;            // Arrays created new
        size_t poolReturns = 0;           // Arrays returned to pool
        size_t poolDiscards = 0;          // Arrays discarded (pool full)
        size_t currentPoolSize = 0;       // Current arrays in pool
        size_t maxPoolSize = 0;           // Maximum pool capacity

        double getHitRate() const {
            return totalAllocations > 0 ?
                   static_cast<double>(poolHits) / totalAllocations : 0.0;
        }
    };

    /**
     * @brief Array pool for efficient reuse of common array sizes
     *
     * Pools arrays by dimension signature to avoid frequent allocations.
     * Focuses on common patterns:
     * - Powers of 2: 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024
     * - Common 2D: 2x2, 3x3, 4x4, 8x8, 16x16, 2x3, 3x2, 4x3, 3x4
     * - Common 1D: lengths up to 1024 that are frequently used
     */
    class ArrayPool
    {
    private:
        struct PoolEntry
        {
            std::vector<std::shared_ptr<FlatMultiArray>> available;
            PoolStats stats;
            size_t maxSize;

            PoolEntry(size_t max = 16) : maxSize(max) {}
        };

        std::unordered_map<std::string, PoolEntry> pools;
        mutable std::mutex poolMutex;
        PoolStats globalStats;

        // Configuration
        static constexpr size_t DEFAULT_POOL_SIZE = 16;
        static constexpr size_t MAX_POOLED_DIMENSION = 1024;
        static constexpr size_t MAX_POOLED_TOTAL_SIZE = 1024 * 1024; // 1M elements max

        /**
         * @brief Generate a unique key for dimension signature
         */
        std::string getDimensionKey(const std::vector<size_t>& dimensions) const
        {
            std::string key;
            for (size_t i = 0; i < dimensions.size(); ++i) {
                if (i > 0) key += "x";
                key += std::to_string(dimensions[i]);
            }
            return key;
        }

        /**
         * @brief Check if dimensions should be pooled
         */
        bool shouldPool(const std::vector<size_t>& dimensions) const
        {
            if (dimensions.empty()) return false;

            // Check if any dimension is too large
            for (size_t dim : dimensions) {
                if (dim > MAX_POOLED_DIMENSION) return false;
            }

            // Check total size
            size_t totalSize = 1;
            for (size_t dim : dimensions) {
                totalSize *= dim;
                if (totalSize > MAX_POOLED_TOTAL_SIZE) return false;
            }

            return isCommonPattern(dimensions);
        }

        /**
         * @brief Check if dimensions match common usage patterns
         */
        bool isCommonPattern(const std::vector<size_t>& dimensions) const
        {
            if (dimensions.size() == 1) {
                // 1D arrays: powers of 2 or common sizes
                size_t size = dimensions[0];
                return isPowerOfTwo(size) ||
                       size <= 32 ||  // Small arrays always pooled
                       size == 50 || size == 100 || size == 200 || size == 500; // Common sizes
            }
            else if (dimensions.size() == 2) {
                size_t rows = dimensions[0];
                size_t cols = dimensions[1];

                // Square matrices (common in math/graphics)
                if (rows == cols) {
                    return rows <= 64 && (isPowerOfTwo(rows) || rows <= 16);
                }

                // Common rectangular patterns
                return (rows <= 16 && cols <= 16) ||  // Small matrices
                       (rows <= 4 && cols <= 32) ||   // Common aspect ratios
                       (rows <= 32 && cols <= 4);
            }
            else if (dimensions.size() == 3) {
                // 3D arrays: small cubes or common patterns
                size_t d1 = dimensions[0], d2 = dimensions[1], d3 = dimensions[2];

                // Cubes
                if (d1 == d2 && d2 == d3) {
                    return d1 <= 8;
                }

                // Common 3D patterns (like RGB images, small volumes)
                return (d1 <= 8 && d2 <= 8 && d3 <= 8) ||
                       (d1 == 3 && d2 <= 16 && d3 <= 16) ||  // RGB-like
                       (d1 <= 16 && d2 <= 16 && d3 == 3);
            }

            return false; // Don't pool higher dimensions
        }

        /**
         * @brief Check if a number is a power of 2
         */
        bool isPowerOfTwo(size_t n) const
        {
            return n > 0 && (n & (n - 1)) == 0;
        }

        /**
         * @brief Clean up pool to prevent memory bloat
         */
        void cleanupPool(PoolEntry& pool)
        {
            while (pool.available.size() > pool.maxSize) {
                pool.available.pop_back();
                pool.stats.poolDiscards++;
                globalStats.poolDiscards++;
            }
        }

    public:
        /**
         * @brief Get singleton instance
         */
        static ArrayPool& getInstance()
        {
            static ArrayPool instance;
            return instance;
        }

        /**
         * @brief Acquire an array from pool or create new
         */
        std::shared_ptr<FlatMultiArray> acquire(const std::vector<size_t>& dimensions,
                                              const Value& defaultValue = std::monostate{})
        {
            std::lock_guard<std::mutex> lock(poolMutex);

            globalStats.totalAllocations++;

            if (!shouldPool(dimensions)) {
                globalStats.poolMisses++;
                return std::make_shared<FlatMultiArray>(dimensions, defaultValue);
            }

            std::string key = getDimensionKey(dimensions);
            auto& pool = pools[key];
            pool.stats.totalAllocations++;

            if (!pool.available.empty()) {
                // Reuse from pool
                auto array = pool.available.back();
                pool.available.pop_back();

                pool.stats.poolHits++;
                globalStats.poolHits++;
                pool.stats.currentPoolSize = pool.available.size();

                // Verify the pooled array has correct dimensions (safety check)
                if (!array || !array->hasDimensions(dimensions)) {
                    // Pool corruption - create new array instead
                    pool.stats.poolMisses++;
                    globalStats.poolMisses++;
                    return std::make_shared<FlatMultiArray>(dimensions, defaultValue);
                }

                // Reset the pooled array with new default value
                array->reset(defaultValue);
                return array;  // Return the reused array, not a new one
            }
            else {
                // Create new
                pool.stats.poolMisses++;
                globalStats.poolMisses++;
                return std::make_shared<FlatMultiArray>(dimensions, defaultValue);
            }
        }

        /**
         * @brief Return an array to the pool
         */
        void release(std::shared_ptr<FlatMultiArray> array)
        {
            if (!array) return;

            auto dimensions = array->getDimensions();
            if (!shouldPool(dimensions)) return;

            std::lock_guard<std::mutex> lock(poolMutex);

            std::string key = getDimensionKey(dimensions);
            auto& pool = pools[key];

            if (pool.available.size() < pool.maxSize) {
                pool.available.push_back(array);
                pool.stats.poolReturns++;
                globalStats.poolReturns++;
                pool.stats.currentPoolSize = pool.available.size();
                pool.stats.maxPoolSize = std::max(pool.stats.maxPoolSize, pool.stats.currentPoolSize);
            }
            else {
                pool.stats.poolDiscards++;
                globalStats.poolDiscards++;
            }
        }

        /**
         * @brief Get global pool statistics
         */
        PoolStats getGlobalStats() const
        {
            std::lock_guard<std::mutex> lock(poolMutex);
            return globalStats;
        }

        /**
         * @brief Get statistics for specific dimension pattern
         */
        PoolStats getPoolStats(const std::vector<size_t>& dimensions) const
        {
            std::lock_guard<std::mutex> lock(poolMutex);
            std::string key = getDimensionKey(dimensions);
            auto it = pools.find(key);
            return it != pools.end() ? it->second.stats : PoolStats{};
        }

        /**
         * @brief Get all pool information for debugging
         */
        std::vector<std::pair<std::string, PoolStats>> getAllPoolStats() const
        {
            std::lock_guard<std::mutex> lock(poolMutex);
            std::vector<std::pair<std::string, PoolStats>> result;
            for (const auto& [key, pool] : pools) {
                result.emplace_back(key, pool.stats);
            }
            return result;
        }

        /**
         * @brief Clear all pools (for testing/cleanup)
         */
        void clear()
        {
            std::lock_guard<std::mutex> lock(poolMutex);
            pools.clear();
            globalStats = PoolStats{};
        }

        /**
         * @brief Configure pool size for specific dimension
         */
        void setPoolSize(const std::vector<size_t>& dimensions, size_t maxSize)
        {
            if (!shouldPool(dimensions)) return;

            std::lock_guard<std::mutex> lock(poolMutex);
            std::string key = getDimensionKey(dimensions);
            pools[key].maxSize = maxSize;
            cleanupPool(pools[key]);
        }
    };
}