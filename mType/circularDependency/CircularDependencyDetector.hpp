#pragma once
#include "CircularDependencyConfig.hpp"
#include "DependencyPatternAnalyzer.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <list>

namespace circularDependency
{
    // LRU cache implementation for validation results
    struct CacheEntry
    {
        bool isValid;
        std::chrono::steady_clock::time_point lastUsed;

        explicit CacheEntry(bool valid) : isValid(valid), lastUsed(std::chrono::steady_clock::now())
        {
        }
    };

    class CircularDependencyDetector
    {
    private:
        CircularDependencyConfig config_;
        std::unique_ptr<DependencyPatternAnalyzer> patternAnalyzer_;

        // Per-type tracking
        std::unordered_map<DependencyType, int> currentDepths_;
        std::unordered_map<DependencyType, std::vector<std::string>> dependencyChains_;
        std::unordered_map<DependencyType, std::unordered_set<std::string>> activeDependencies_;


        mutable std::unordered_map<std::string, CacheEntry> validationCache_;
        mutable std::list<std::string> cacheAccessOrder_; // For LRU tracking
        mutable std::unordered_map<std::string, std::list<std::string>::iterator> cacheOrderMap_;
        // Fast access to list positions

    public:
        
        explicit CircularDependencyDetector();
        
        explicit CircularDependencyDetector(const CircularDependencyConfig& config);

       
        bool enterDependency(DependencyType type, const std::string& identifier,
                             const std::string& location = "");

        
        void exitDependency(DependencyType type, const std::string& identifier);

       
        bool wouldExceedDepthLimit(DependencyType type) const;

        
        std::vector<std::string> getDependencyChain(DependencyType type) const;

      
        int getCurrentDepth(DependencyType type) const;
        
        void resetDependencyType(DependencyType type);

        
        void resetAll();

      
        const CircularDependencyConfig& getConfig() const { return config_; }

       
        void setConfig(const CircularDependencyConfig& config);

       
        bool validateChain(DependencyType type, const std::vector<std::string>& chain,
                           const std::string& location = "");
        
        std::string createCacheKey(DependencyType type, const std::vector<std::string>& chain) const;
        
        void clearCache() const;

    private:
      
        void initialize();
        
        bool checkTrueCycle(DependencyType type, const std::string& identifier) const;

        
        static std::string getDependencyTypeName(DependencyType type);

        // LRU cache management helpers
        void updateCacheAccess(const std::string& key) const;
        void evictLRUEntries() const;
        bool getCachedResult(const std::string& key) const;
        void setCachedResult(const std::string& key, bool result) const;
    };
}
