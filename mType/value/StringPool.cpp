#include "StringPool.hpp"
#include <cstddef>
#include <algorithm>

namespace value
{
    InternedString StringPool::intern(const std::string& str)
    {
        if (!shouldIntern(str)) {
            stats.totalRequests++;
            stats.poolMisses++;
            return InternedString{};
        }
        
        stats.totalRequests++;

        auto it = stringToId.find(str);
        if (it != stringToId.end()) {
            stats.poolHits++;
            size_t id = it->second;
            auto& entry = idToEntry[id];
            entry->lastAccessed = std::chrono::steady_clock::now();
            entry->refCount.fetch_add(1);
            return InternedString(id, this);
        }

        stats.poolMisses++;
        size_t id = nextId.fetch_add(1);
        auto entry = std::make_unique<PoolEntry>(str);
        stats.totalMemorySaved += str.length();

        stringToId[str] = id;
        idToEntry[id] = std::move(entry);

        return InternedString(id, this);
    }

    InternedString StringPool::intern(std::string&& str)
    {
        if (!shouldIntern(str)) {
            stats.totalRequests++;
            stats.poolMisses++;
            return InternedString{};
        }
        
        stats.totalRequests++;

        auto it = stringToId.find(str);
        if (it != stringToId.end()) {
            stats.poolHits++;
            size_t id = it->second;
            auto& entry = idToEntry[id];
            entry->lastAccessed = std::chrono::steady_clock::now();
            entry->refCount.fetch_add(1);
            return InternedString(id, this);
        }

        stats.poolMisses++;
        size_t id = nextId.fetch_add(1);
        size_t strLen = str.length();

        auto entry = std::make_unique<PoolEntry>(std::move(str));
        stats.totalMemorySaved += strLen;

        stringToId[entry->value] = id;
        idToEntry[id] = std::move(entry);

        return InternedString(id, this);
    }

    InternedString StringPool::getById(size_t id) const
    {
        auto it = idToEntry.find(id);
        if (it != idToEntry.end()) {
            it->second->refCount.fetch_add(1);
            it->second->lastAccessed = std::chrono::steady_clock::now();
            return InternedString(id, const_cast<StringPool*>(this));
        }
        return InternedString{};
    }

    void StringPool::incrementRef(size_t id)
    {
        auto it = idToEntry.find(id);
        if (it != idToEntry.end()) {
            it->second->refCount.fetch_add(1);
            it->second->lastAccessed = std::chrono::steady_clock::now();
        }
    }

    void StringPool::decrementRef(size_t id)
    {
        auto it = idToEntry.find(id);
        if (it != idToEntry.end()) {
            size_t refs = it->second->refCount.fetch_sub(1);
            if (refs == 1) {
                // Save string value before erasing to prevent use-after-free
                // and ensure exception safety
                std::string value = it->second->value;

                // Erase from idToEntry first - this invalidates iterator
                idToEntry.erase(it);

                // Then erase from stringToId using saved value
                // If this throws, at least idToEntry is already cleaned up
                stringToId.erase(value);
            }
        }
    }

    const std::string& StringPool::getStringById(size_t id) const
    {
        auto it = idToEntry.find(id);
        if (it != idToEntry.end()) {
            return it->second->value;
        }
        static const std::string empty;
        return empty;
    }

    void StringPool::cleanupExpiredEntries()
    {
        auto now = std::chrono::steady_clock::now();
        auto expiredThreshold = now - CLEANUP_INTERVAL;

        auto it = idToEntry.begin();
        while (it != idToEntry.end()) {
            if (it->second->refCount.load() == 0 &&
                it->second->lastAccessed < expiredThreshold) {
                stringToId.erase(it->second->value);
                it = idToEntry.erase(it);
            } else {
                ++it;
            }
        }
    }

    size_t StringPool::getTotalMemoryUsage() const
    {
        size_t total = 0;

        for (const auto& pair : idToEntry) {
            total += getEntryMemoryUsage(*pair.second);
        }

        total += stringToId.size() * (sizeof(std::string) + sizeof(size_t));
        total += idToEntry.size() * (sizeof(size_t) + sizeof(std::unique_ptr<PoolEntry>));

        return total;
    }

    std::vector<std::pair<std::string, size_t>> StringPool::getTopStrings(size_t count) const
    {
        std::vector<std::pair<std::string, size_t>> result;

        for (const auto& pair : idToEntry) {
            result.emplace_back(pair.second->value, pair.second->refCount.load());
        }

        std::sort(result.begin(), result.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        if (result.size() > count) {
            result.resize(count);
        }

        return result;
    }
}