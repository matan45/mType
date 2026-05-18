#include "StringCache.hpp"
#include "InternedString.hpp"
#include "StringPool.hpp"
#include "../environment/registry/ClassDefinition.hpp"

namespace value
{
    StringCache& StringCache::getInstance()
    {
        static StringCache instance;
        return instance;
    }

    void StringCache::initialize()
    {
        auto& instance = getInstance();
        instance.initialized_ = true;
    }

    std::shared_ptr<runtimeTypes::klass::ObjectInstance> StringCache::createCachedString(
        const std::string& value,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> stringClassDef)
    {
        if (!stringClassDef)
        {
            return nullptr;
        }
        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(
            stringClassDef,
            std::unordered_map<std::string, std::string>{});
        instance->setField("value", value);
        return instance;
    }

    std::shared_ptr<runtimeTypes::klass::ObjectInstance> StringCache::getString(
        const std::string& value,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> stringClassDef)
    {
        auto& instance = getInstance();
        if (!instance.initialized_)
        {
            initialize();
        }

        if (value.empty())
        {
            if (!instance.emptyInstance_ && stringClassDef)
            {
                instance.emptyInstance_ = createCachedString(value, stringClassDef);
            }
            return instance.emptyInstance_;
        }

        // StringPool gates by length (MIN/MAX_INTERN_LENGTH). If it returns
        // an empty handle, the content is out of intern range — fall back
        // to a fresh allocation rather than caching by std::string content
        // (which would duplicate the pool's eligibility logic).
        auto interned = StringPool::getInstance().intern(value);
        if (interned.empty()) return nullptr;
        const size_t id = interned.getPoolId();

        auto it = instance.cache_.find(id);
        if (it != instance.cache_.end())
        {
            return it->second;
        }

        if (!stringClassDef) return nullptr;
        auto created = createCachedString(value, stringClassDef);
        if (!created) return nullptr;

        if (instance.cache_.size() >= kMaxEntries)
        {
            const size_t evictId = instance.insertOrder_.front();
            instance.cache_.erase(evictId);
            instance.insertOrder_.pop_front();
        }
        instance.cache_.emplace(id, created);
        instance.insertOrder_.push_back(id);
        return created;
    }

    void StringCache::clear()
    {
        auto& instance = getInstance();
        instance.emptyInstance_.reset();
        instance.cache_.clear();
        instance.insertOrder_.clear();
        instance.initialized_ = false;
    }
}
