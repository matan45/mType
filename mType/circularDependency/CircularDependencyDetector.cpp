#include "CircularDependencyDetector.hpp"
#include "TrueCyclicException.hpp"
#include "DepthLimitException.hpp"
#include <sstream>

namespace circularDependency
{
    CircularDependencyDetector::CircularDependencyDetector()
        : config_{}, patternAnalyzer_{std::make_unique<DependencyPatternAnalyzer>(config_)}
    {
        initialize();
    }

    CircularDependencyDetector::CircularDependencyDetector(const CircularDependencyConfig& config)
        : config_(config), patternAnalyzer_{std::make_unique<DependencyPatternAnalyzer>(config_)}
    {
        initialize();
    }

    void CircularDependencyDetector::initialize()
    {
        // Initialize tracking for all dependency types
        for (int i = 0; i <= static_cast<int>(DependencyType::STATIC_FIELD_INITIALIZATION); ++i)
        {
            DependencyType type = static_cast<DependencyType>(i);
            currentDepths_[type] = 0;
            dependencyChains_[type] = {};
            activeDependencies_[type] = {};
        }
    }

    std::vector<std::string> CircularDependencyDetector::buildChainWithIdentifier(
        DependencyType type, const std::string& identifier) const
    {
        const auto& existingChain = dependencyChains_.at(type);
        std::vector<std::string> chain;
        chain.reserve(existingChain.size() + 1);  // Pre-allocate to avoid reallocation
        chain = existingChain;
        chain.push_back(identifier);
        return chain;
    }

    void CircularDependencyDetector::cacheNegativeResult(
        DependencyType type, const std::vector<std::string>& chain) const
    {
        if (config_.enableCaching)
        {
            std::string cacheKey = createCacheKey(type, chain);
            setCachedResult(cacheKey, false);
        }
    }

    void CircularDependencyDetector::updateDependencyState(
        DependencyType type, const std::string& identifier)
    {
        dependencyChains_[type].push_back(identifier);
        activeDependencies_[type].insert(identifier);
        currentDepths_[type]++;
    }

    bool CircularDependencyDetector::enterDependency(DependencyType type, const std::string& identifier,
                                                     const std::string& location)
    {
        try
        {
            // Build chain once for all checks (optimization)
            auto chainWithIdentifier = buildChainWithIdentifier(type, identifier);

            // Check cache first
            if (config_.enableCaching)
            {
                std::string cacheKey = createCacheKey(type, chainWithIdentifier);

                auto cacheIt = validationCache_.find(cacheKey);
                if (cacheIt != validationCache_.end())
                {
                    updateCacheAccess(cacheKey);
                    if (!cacheIt->second.isValid)
                    {
                        // Previously detected as problematic
                        throw TrueCyclicException(type, identifier, chainWithIdentifier, location);
                    }
                    // Cache says it's safe, but still need to update current state
                }
            }

            // Check for true circular dependency first
            if (checkTrueCycle(type, identifier))
            {
                cacheNegativeResult(type, chainWithIdentifier);
                throw TrueCyclicException(type, identifier, chainWithIdentifier, location);
            }

            // Check depth limit
            if (currentDepths_[type] >= config_.getMaxDepth(type))
            {
                cacheNegativeResult(type, chainWithIdentifier);
                throw DepthLimitException(type, currentDepths_[type], config_.getMaxDepth(type), chainWithIdentifier, location);
            }

            // Early pattern detection
            if (config_.enableEarlyDetection)
            {
                if (patternAnalyzer_->detectAnyPattern(chainWithIdentifier))
                {
                    cacheNegativeResult(type, chainWithIdentifier);
                    std::string suggestion = patternAnalyzer_->suggestSimplification(chainWithIdentifier);
                    throw TrueCyclicException(type, identifier, chainWithIdentifier,
                                              location + (suggestion.empty() ? "" : "\nSuggestion: " + suggestion));
                }
            }

            // Safe to proceed - update tracking state
            updateDependencyState(type, identifier);

            // Update cache
            if (config_.enableCaching)
            {
                std::string cacheKey = createCacheKey(type, dependencyChains_[type]);
                setCachedResult(cacheKey, true);
            }

            return true;
        }
        catch (...)
        {
            // Don't update state if validation failed
            throw;
        }
    }

    void CircularDependencyDetector::exitDependency(DependencyType type, const std::string& identifier)
    {
        // Remove from active set
        activeDependencies_[type].erase(identifier);

        // Remove from chain (should be at the end)
        auto& chain = dependencyChains_[type];
        if (!chain.empty() && chain.back() == identifier)
        {
            chain.pop_back();
        }

        // Decrease depth
        if (currentDepths_[type] > 0)
        {
            currentDepths_[type]--;
        }
    }

    bool CircularDependencyDetector::wouldExceedDepthLimit(DependencyType type) const
    {
        auto it = currentDepths_.find(type);
        int currentDepth = (it != currentDepths_.end()) ? it->second : 0;
        return currentDepth >= config_.getMaxDepth(type);
    }

    std::vector<std::string> CircularDependencyDetector::getDependencyChain(DependencyType type) const
    {
        auto it = dependencyChains_.find(type);
        return it != dependencyChains_.end() ? it->second : std::vector<std::string>{};
    }

    int CircularDependencyDetector::getCurrentDepth(DependencyType type) const
    {
        auto it = currentDepths_.find(type);
        return it != currentDepths_.end() ? it->second : 0;
    }

    void CircularDependencyDetector::resetDependencyType(DependencyType type)
    {
        currentDepths_[type] = 0;
        dependencyChains_[type].clear();
        activeDependencies_[type].clear();
    }

    void CircularDependencyDetector::resetAll()
    {
        for (int i = 0; i <= static_cast<int>(DependencyType::STATIC_FIELD_INITIALIZATION); ++i)
        {
            resetDependencyType(static_cast<DependencyType>(i));
        }
        clearCache();
    }

    void CircularDependencyDetector::setConfig(const CircularDependencyConfig& config)
    {
        config_ = config;
        patternAnalyzer_ = std::make_unique<DependencyPatternAnalyzer>(config_);
        clearCache(); // Clear cache as limits may have changed
    }


    bool CircularDependencyDetector::validateChain(DependencyType type, const std::vector<std::string>& chain,
                                                   const std::string& location)
    {
        // Check for cycles in the chain
        std::unordered_set<std::string> seen;
        for (const auto& item : chain)
        {
            if (seen.find(item) != seen.end())
            {
                throw TrueCyclicException(type, item, chain, location);
            }
            seen.insert(item);
        }

        // Check depth limit
        if (static_cast<int>(chain.size()) > config_.getMaxDepth(type))
        {
            throw DepthLimitException(type, static_cast<int>(chain.size()), config_.getMaxDepth(type), chain, location);
        }

        // Check patterns if enabled
        if (config_.enableEarlyDetection && patternAnalyzer_->detectAnyPattern(chain))
        {
            std::string suggestion = patternAnalyzer_->suggestSimplification(chain);
            throw TrueCyclicException(type, chain.empty() ? "" : chain.back(), chain,
                                      location + (suggestion.empty() ? "" : "\nSuggestion: " + suggestion));
        }

        return true;
    }

    std::string CircularDependencyDetector::createCacheKey(DependencyType type,
                                                           const std::vector<std::string>& chain) const
    {
        std::ostringstream oss;
        oss << static_cast<int>(type) << ":";
        for (size_t i = 0; i < chain.size(); ++i)
        {
            if (i > 0) oss << "->";
            oss << chain[i];
        }
        return oss.str();
    }

    void CircularDependencyDetector::clearCache() const
    {
        validationCache_.clear();
        cacheAccessOrder_.clear();
        cacheOrderMap_.clear();
    }

    void CircularDependencyDetector::updateCacheAccess(const std::string& key) const
    {
        // Update access time
        auto cacheIt = validationCache_.find(key);
        if (cacheIt != validationCache_.end())
        {
            cacheIt->second.lastUsed = std::chrono::steady_clock::now();

            // Move to front of LRU list (most recently used)
            auto orderIt = cacheOrderMap_.find(key);
            if (orderIt != cacheOrderMap_.end())
            {
                cacheAccessOrder_.erase(orderIt->second);
            }
            cacheAccessOrder_.push_front(key);
            cacheOrderMap_[key] = cacheAccessOrder_.begin();
        }
    }

    void CircularDependencyDetector::evictLRUEntries() const
    {
        // Evict entries until we're under the cache size limit
        while (validationCache_.size() > static_cast<size_t>(config_.cacheMaxSize * 0.8))
        {
            if (cacheAccessOrder_.empty()) break;

            // Remove least recently used entry
            const std::string& lruKey = cacheAccessOrder_.back();
            validationCache_.erase(lruKey);
            cacheOrderMap_.erase(lruKey);
            cacheAccessOrder_.pop_back();
        }
    }

    void CircularDependencyDetector::setCachedResult(const std::string& key, bool result) const
    {
        // Check if we need to evict entries first
        if (validationCache_.size() >= static_cast<size_t>(config_.cacheMaxSize))
        {
            evictLRUEntries();
        }

        // Add new entry
        validationCache_.emplace(key, CacheEntry(result));
        cacheAccessOrder_.push_front(key);
        cacheOrderMap_[key] = cacheAccessOrder_.begin();
    }

    bool CircularDependencyDetector::checkTrueCycle(DependencyType type, const std::string& identifier) const
    {
        auto it = activeDependencies_.find(type);
        if (it == activeDependencies_.end())
        {
            return false;
        }
        return it->second.find(identifier) != it->second.end();
    }
}
