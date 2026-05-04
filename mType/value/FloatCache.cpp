#include "FloatCache.hpp"
#include <cstddef>
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include <cstring>

namespace value
{
    const std::array<double, FloatCache::CACHE_SIZE> FloatCache::kCachedValues = {
        0.0, 1.0, -1.0, 0.5, -0.5, 2.0
    };

    namespace
    {
        // Bitwise reinterpret without invoking std::bit_cast (MSVC v145
        // tracks the C++20 header; memcpy lowers to the same single mov on
        // optimized builds and works on every supported toolset).
        uint64_t bitsOf(double v) noexcept
        {
            uint64_t r = 0;
            std::memcpy(&r, &v, sizeof(r));
            return r;
        }
    }

    FloatCache& FloatCache::getInstance()
    {
        static FloatCache instance;
        return instance;
    }

    void FloatCache::initialize()
    {
        auto& instance = getInstance();
        instance.initialized_ = true;
    }

    int FloatCache::cacheIndexOf(double value)
    {
        const uint64_t bits = bitsOf(value);
        for (size_t i = 0; i < CACHE_SIZE; ++i)
        {
            if (bits == bitsOf(kCachedValues[i]))
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    bool FloatCache::isCacheable(double value)
    {
        return cacheIndexOf(value) >= 0;
    }

    std::shared_ptr<runtimeTypes::klass::ObjectInstance> FloatCache::createCachedFloat(
        double value,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> floatClassDef)
    {
        if (!floatClassDef)
        {
            return nullptr;
        }
        auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(
            floatClassDef,
            std::unordered_map<std::string, std::string>{});
        instance->setField("value", value);
        return instance;
    }

    std::shared_ptr<runtimeTypes::klass::ObjectInstance> FloatCache::getFloat(
        double value,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> floatClassDef)
    {
        auto& instance = getInstance();
        if (!instance.initialized_)
        {
            initialize();
        }

        const int idx = cacheIndexOf(value);
        if (idx < 0) return nullptr;

        auto& slot = instance.cache_[idx];
        if (!slot && floatClassDef)
        {
            slot = createCachedFloat(value, floatClassDef);
        }
        return slot;
    }

    void FloatCache::clear()
    {
        auto& instance = getInstance();
        instance.cache_.fill(nullptr);
        instance.initialized_ = false;
    }
}
