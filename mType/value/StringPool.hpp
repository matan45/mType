#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>

namespace value
{
    struct StringPoolStats
    {
        size_t totalRequests = 0;
        size_t poolHits = 0;
        size_t poolMisses = 0;
        size_t currentPoolSize = 0;
        size_t totalMemorySaved = 0;
        size_t maxPoolSize = 0;

        double getHitRate() const {
            return totalRequests > 0 ?
                   static_cast<double>(poolHits) / totalRequests : 0.0;
        }

        double getMemoryEfficiency() const {
            return currentPoolSize > 0 ?
                   static_cast<double>(totalMemorySaved) / currentPoolSize : 0.0;
        }
    };

    class StringPool;

    class InternedString
    {
    private:
        size_t poolId;
        StringPool* pool;

        friend class StringPool;

        InternedString(size_t id, StringPool* p) : poolId(id), pool(p) {}

    public:
        InternedString() : poolId(0), pool(nullptr) {}

        InternedString(const InternedString& other);

        InternedString(InternedString&& other) noexcept
            : poolId(other.poolId), pool(other.pool) {
            other.poolId = 0;
            other.pool = nullptr;
        }

        InternedString& operator=(const InternedString& other);
        InternedString& operator=(InternedString&& other) noexcept;

        ~InternedString();

        const std::string& getString() const;

        operator const std::string&() const { return getString(); }

        bool operator==(const InternedString& other) const {
            return poolId == other.poolId && pool == other.pool;
        }

        bool operator!=(const InternedString& other) const {
            return !(*this == other);
        }

        bool operator==(const std::string& str) const {
            return getString() == str;
        }

        bool operator!=(const std::string& str) const {
            return getString() != str;
        }

        // String concatenation operators
        std::string operator+(const std::string& str) const {
            return getString() + str;
        }

        std::string operator+(const InternedString& other) const {
            return getString() + other.getString();
        }

        size_t hash() const noexcept { return poolId; }

        bool empty() const { return poolId == 0; }
        size_t size() const { return getString().size(); }
        size_t length() const { return size(); }

        size_t getPoolId() const { return poolId; }
    };

    class StringPool
    {
    private:
        struct PoolEntry
        {
            std::string value;
            std::atomic<size_t> refCount;
            std::chrono::steady_clock::time_point lastAccessed;

            PoolEntry(const std::string& str)
                : value(str), refCount(1), lastAccessed(std::chrono::steady_clock::now()) {}

            PoolEntry(std::string&& str)
                : value(std::move(str)), refCount(1), lastAccessed(std::chrono::steady_clock::now()) {}
        };

        std::unordered_map<std::string, size_t> stringToId;
        std::unordered_map<size_t, std::unique_ptr<PoolEntry>> idToEntry;
        mutable std::mutex poolMutex;
        std::atomic<size_t> nextId{1};
        StringPoolStats stats;

        static constexpr size_t MIN_INTERN_LENGTH = 1;
        static constexpr size_t MAX_INTERN_LENGTH = 1024;
        static constexpr size_t MAX_POOL_SIZE = 100000;
        static constexpr std::chrono::minutes CLEANUP_INTERVAL{30};

        bool shouldIntern(const std::string& str) const
        {
            size_t len = str.length();
            return len >= MIN_INTERN_LENGTH && len <= MAX_INTERN_LENGTH;
        }

        void cleanupExpiredEntries();

        size_t getEntryMemoryUsage(const PoolEntry& entry) const
        {
            return sizeof(PoolEntry) + entry.value.capacity();
        }

        friend class InternedString;

        void incrementRef(size_t id);
        void decrementRef(size_t id);
        const std::string& getStringById(size_t id) const;

    private:
        // Private constructor for singleton
        StringPool() = default;

        // Delete copy constructor and assignment
        StringPool(const StringPool&) = delete;
        StringPool& operator=(const StringPool&) = delete;

    public:
        static StringPool& getInstance()
        {
            static StringPool instance;
            return instance;
        }

        InternedString intern(const std::string& str);
        InternedString intern(std::string&& str);

        bool contains(const std::string& str) const
        {
            std::lock_guard<std::mutex> lock(poolMutex);
            return stringToId.find(str) != stringToId.end();
        }

        InternedString getById(size_t id) const;

        StringPoolStats getStats() const
        {
            std::lock_guard<std::mutex> lock(poolMutex);
            auto currentStats = stats;
            currentStats.currentPoolSize = idToEntry.size();
            currentStats.maxPoolSize = std::max(currentStats.maxPoolSize, currentStats.currentPoolSize);
            return currentStats;
        }

        void clear()
        {
            std::lock_guard<std::mutex> lock(poolMutex);
            stringToId.clear();
            idToEntry.clear();
            stats = StringPoolStats{};
            nextId.store(1);
        }

        void cleanup()
        {
            std::lock_guard<std::mutex> lock(poolMutex);
            cleanupExpiredEntries();
        }

        size_t getTotalMemoryUsage() const;
        std::vector<std::pair<std::string, size_t>> getTopStrings(size_t count = 10) const;
    };
}

// Global operators for string concatenation
namespace value
{
    inline std::string operator+(const std::string& lhs, const InternedString& rhs)
    {
        return lhs + rhs.getString();
    }
}

namespace std
{
    template<>
    struct hash<value::InternedString>
    {
        size_t operator()(const value::InternedString& is) const noexcept
        {
            return is.hash();
        }
    };
}