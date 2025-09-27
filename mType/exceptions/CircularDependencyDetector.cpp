#include "CircularDependencyDetector.hpp"
#include <sstream>
#include <algorithm>

namespace mtype::exceptions {

    CircularDependencyDetector::CircularDependencyDetector()
        : config_{}, patternAnalyzer_{std::make_unique<DependencyPatternAnalyzer>(config_)} {
        initialize();
    }

    CircularDependencyDetector::CircularDependencyDetector(const CircularDependencyConfig& config)
        : config_(config), patternAnalyzer_{std::make_unique<DependencyPatternAnalyzer>(config_)} {
        initialize();
    }

    void CircularDependencyDetector::initialize() {
        // Initialize tracking for all dependency types
        for (int i = 0; i <= static_cast<int>(DependencyType::METHOD_OVERLOAD); ++i) {
            DependencyType type = static_cast<DependencyType>(i);
            currentDepths_[type] = 0;
            dependencyChains_[type] = {};
            activeDependencies_[type] = {};
        }
    }

    bool CircularDependencyDetector::enterDependency(DependencyType type, const std::string& identifier,
                                                    const std::string& location) {
        auto startTime = std::chrono::high_resolution_clock::now();

        try {
            // Check cache first
            if (config_.enableCaching) {
                auto chain = dependencyChains_[type];
                chain.push_back(identifier);
                std::string cacheKey = createCacheKey(type, chain);

                auto cacheIt = validationCache_.find(cacheKey);
                if (cacheIt != validationCache_.end()) {
                    metrics_.addCacheHit();
                    if (!cacheIt->second) {
                        // Previously detected as problematic
                        throw TrueCyclicException(type, identifier, chain, location);
                    }
                    // Cache says it's safe, but still need to update current state
                }
                else {
                    metrics_.addCacheMiss();
                }
            }

            // Check for true circular dependency first
            if (checkTrueCycle(type, identifier)) {
                auto chain = dependencyChains_[type];
                chain.push_back(identifier);

                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                recordDetection(false, true, false, duration);

                throw TrueCyclicException(type, identifier, chain, location);
            }

            // Check depth limit
            if (currentDepths_[type] >= config_.getMaxDepth(type)) {
                auto chain = dependencyChains_[type];
                chain.push_back(identifier);

                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                recordDetection(true, false, false, duration);

                throw DepthLimitException(type, currentDepths_[type], config_.getMaxDepth(type), chain, location);
            }

            // Early pattern detection
            if (config_.enableEarlyDetection) {
                auto currentChain = dependencyChains_[type];
                currentChain.push_back(identifier);

                if (patternAnalyzer_->detectAnyPattern(currentChain)) {
                    auto endTime = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                    recordDetection(false, false, true, duration);

                    std::string suggestion = patternAnalyzer_->suggestSimplification(currentChain);
                    throw TrueCyclicException(type, identifier, currentChain,
                                            location + (suggestion.empty() ? "" : "\nSuggestion: " + suggestion));
                }
            }

            // Safe to proceed - update tracking state
            dependencyChains_[type].push_back(identifier);
            activeDependencies_[type].insert(identifier);
            currentDepths_[type]++;

            // Update cache
            if (config_.enableCaching) {
                std::string cacheKey = createCacheKey(type, dependencyChains_[type]);
                if (validationCache_.size() < static_cast<size_t>(config_.cacheMaxSize)) {
                    validationCache_[cacheKey] = true;
                }
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            recordDetection(false, false, false, duration);

            return true;
        }
        catch (...) {
            // Don't update state if validation failed
            throw;
        }
    }

    void CircularDependencyDetector::exitDependency(DependencyType type, const std::string& identifier) {
        // Remove from active set
        activeDependencies_[type].erase(identifier);

        // Remove from chain (should be at the end)
        auto& chain = dependencyChains_[type];
        if (!chain.empty() && chain.back() == identifier) {
            chain.pop_back();
        }

        // Decrease depth
        if (currentDepths_[type] > 0) {
            currentDepths_[type]--;
        }
    }

    bool CircularDependencyDetector::wouldExceedDepthLimit(DependencyType type) const {
        auto it = currentDepths_.find(type);
        int currentDepth = (it != currentDepths_.end()) ? it->second : 0;
        return currentDepth >= config_.getMaxDepth(type);
    }

    std::vector<std::string> CircularDependencyDetector::getDependencyChain(DependencyType type) const {
        auto it = dependencyChains_.find(type);
        return it != dependencyChains_.end() ? it->second : std::vector<std::string>{};
    }

    int CircularDependencyDetector::getCurrentDepth(DependencyType type) const {
        auto it = currentDepths_.find(type);
        return it != currentDepths_.end() ? it->second : 0;
    }

    void CircularDependencyDetector::resetDependencyType(DependencyType type) {
        currentDepths_[type] = 0;
        dependencyChains_[type].clear();
        activeDependencies_[type].clear();
    }

    void CircularDependencyDetector::resetAll() {
        for (auto& pair : currentDepths_) {
            pair.second = 0;
        }
        for (auto& pair : dependencyChains_) {
            pair.second.clear();
        }
        for (auto& pair : activeDependencies_) {
            pair.second.clear();
        }
        clearCache();
    }

    void CircularDependencyDetector::setConfig(const CircularDependencyConfig& config) {
        config_ = config;
        patternAnalyzer_ = std::make_unique<DependencyPatternAnalyzer>(config_);
        clearCache(); // Clear cache as limits may have changed
    }

    void CircularDependencyDetector::resetMetrics() {
        metrics_ = DependencyDetectionMetrics{};
    }

    bool CircularDependencyDetector::validateChain(DependencyType type, const std::vector<std::string>& chain,
                                                  const std::string& location) {
        // Check for cycles in the chain
        std::unordered_set<std::string> seen;
        for (const auto& item : chain) {
            if (seen.find(item) != seen.end()) {
                throw TrueCyclicException(type, item, chain, location);
            }
            seen.insert(item);
        }

        // Check depth limit
        if (static_cast<int>(chain.size()) > config_.getMaxDepth(type)) {
            throw DepthLimitException(type, static_cast<int>(chain.size()), config_.getMaxDepth(type), chain, location);
        }

        // Check patterns if enabled
        if (config_.enableEarlyDetection && patternAnalyzer_->detectAnyPattern(chain)) {
            std::string suggestion = patternAnalyzer_->suggestSimplification(chain);
            throw TrueCyclicException(type, chain.empty() ? "" : chain.back(), chain,
                                    location + (suggestion.empty() ? "" : "\nSuggestion: " + suggestion));
        }

        return true;
    }

    std::string CircularDependencyDetector::createCacheKey(DependencyType type, const std::vector<std::string>& chain) const {
        std::ostringstream oss;
        oss << static_cast<int>(type) << ":";
        for (size_t i = 0; i < chain.size(); ++i) {
            if (i > 0) oss << "->";
            oss << chain[i];
        }
        return oss.str();
    }

    void CircularDependencyDetector::clearCache() const {
        validationCache_.clear();
    }

    bool CircularDependencyDetector::checkTrueCycle(DependencyType type, const std::string& identifier) const {
        auto it = activeDependencies_.find(type);
        if (it == activeDependencies_.end()) {
            return false;
        }
        return it->second.find(identifier) != it->second.end();
    }

    void CircularDependencyDetector::recordDetection(bool wasDepthLimit, bool wasTrueCycle, bool wasEarlyPattern,
                                                    std::chrono::milliseconds detectionTime) const {
        if (config_.enablePerformanceMetrics) {
            metrics_.addDetection(detectionTime, wasDepthLimit, wasTrueCycle, wasEarlyPattern);
        }
    }

    std::string CircularDependencyDetector::getDependencyTypeName(DependencyType type) {
        switch (type) {
            case DependencyType::GENERIC_SUBSTITUTION: return "generic_substitution";
            case DependencyType::IMPORT_CHAIN: return "import_chain";
            case DependencyType::INTERFACE_INHERITANCE: return "interface_inheritance";
            case DependencyType::CLASS_INHERITANCE: return "class_inheritance";
            case DependencyType::METHOD_OVERLOAD: return "method_overload";
            default: return "unknown";
        }
    }

} // namespace mtype::exceptions