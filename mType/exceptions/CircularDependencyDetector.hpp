#pragma once

#include "CircularDependencyDetection.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <memory>
#include <chrono>

namespace mtype::exceptions {

    /**
     * @brief Main circular dependency detection service
     *
     * This class provides a unified interface for detecting circular dependencies
     * across different types of relationships in the mType language system.
     */
    class CircularDependencyDetector {
    private:
        CircularDependencyConfig config_;
        std::unique_ptr<DependencyPatternAnalyzer> patternAnalyzer_;

        // Per-type tracking
        std::unordered_map<DependencyType, int> currentDepths_;
        std::unordered_map<DependencyType, std::vector<std::string>> dependencyChains_;
        std::unordered_map<DependencyType, std::unordered_set<std::string>> activeDependencies_;

        // Performance metrics
        mutable DependencyDetectionMetrics metrics_;

        // Validation cache for performance
        mutable std::unordered_map<std::string, bool> validationCache_;

    public:
        /**
         * @brief Construct detector with default configuration
         */
        CircularDependencyDetector();

        /**
         * @brief Construct detector with custom configuration
         */
        explicit CircularDependencyDetector(const CircularDependencyConfig& config);

        /**
         * @brief Enter a new dependency step
         * @param type Type of dependency being processed
         * @param identifier Unique identifier for this dependency step
         * @param location Optional source location for error reporting
         * @return true if safe to proceed, throws exception if circular dependency detected
         */
        bool enterDependency(DependencyType type, const std::string& identifier,
                           const std::string& location = "");

        /**
         * @brief Exit current dependency step
         * @param type Type of dependency being exited
         * @param identifier Identifier that was entered
         */
        void exitDependency(DependencyType type, const std::string& identifier);

        /**
         * @brief Check if adding dependency would exceed depth limit
         * @param type Type of dependency to check
         * @return true if would exceed limit
         */
        bool wouldExceedDepthLimit(DependencyType type) const;

        /**
         * @brief Get current dependency chain for a type
         * @param type Type of dependency
         * @return Current chain of dependencies
         */
        std::vector<std::string> getDependencyChain(DependencyType type) const;

        /**
         * @brief Get current depth for a dependency type
         * @param type Type of dependency
         * @return Current depth
         */
        int getCurrentDepth(DependencyType type) const;

        /**
         * @brief Reset all dependency tracking for a type
         * @param type Type to reset
         */
        void resetDependencyType(DependencyType type);

        /**
         * @brief Reset all dependency tracking
         */
        void resetAll();

        /**
         * @brief Get current configuration
         */
        const CircularDependencyConfig& getConfig() const { return config_; }

        /**
         * @brief Update configuration
         */
        void setConfig(const CircularDependencyConfig& config);

        /**
         * @brief Get performance metrics
         */
        const DependencyDetectionMetrics& getMetrics() const { return metrics_; }

        /**
         * @brief Reset performance metrics
         */
        void resetMetrics();

        /**
         * @brief Validate a complete dependency chain (for testing/analysis)
         * @param type Type of dependency
         * @param chain Complete chain to validate
         * @param location Optional source location
         * @return true if chain is valid, throws exception if circular
         */
        bool validateChain(DependencyType type, const std::vector<std::string>& chain,
                          const std::string& location = "");

        /**
         * @brief Create a cached key for validation results
         */
        std::string createCacheKey(DependencyType type, const std::vector<std::string>& chain) const;

        /**
         * @brief Clear validation cache
         */
        void clearCache() const;

    private:
        /**
         * @brief Initialize the detector with given config
         */
        void initialize();

        /**
         * @brief Check for true circular dependency
         * @param type Type of dependency
         * @param identifier Current identifier
         * @return true if circular dependency detected
         */
        bool checkTrueCycle(DependencyType type, const std::string& identifier) const;

        /**
         * @brief Record detection metrics
         */
        void recordDetection(bool wasDepthLimit, bool wasTrueCycle, bool wasEarlyPattern,
                           std::chrono::milliseconds detectionTime) const;

        /**
         * @brief Get dependency type name for error messages
         */
        static std::string getDependencyTypeName(DependencyType type);
    };

    /**
     * @brief RAII helper for automatic dependency tracking
     */
    class DependencyScope {
    private:
        CircularDependencyDetector& detector_;
        DependencyType type_;
        std::string identifier_;
        bool entered_;

    public:
        DependencyScope(CircularDependencyDetector& detector, DependencyType type,
                       const std::string& identifier, const std::string& location = "")
            : detector_(detector), type_(type), identifier_(identifier), entered_(false) {
            entered_ = detector_.enterDependency(type_, identifier_, location);
        }

        ~DependencyScope() {
            if (entered_) {
                detector_.exitDependency(type_, identifier_);
            }
        }

        // Non-copyable, non-movable
        DependencyScope(const DependencyScope&) = delete;
        DependencyScope& operator=(const DependencyScope&) = delete;
        DependencyScope(DependencyScope&&) = delete;
        DependencyScope& operator=(DependencyScope&&) = delete;
    };

} // namespace mtype::exceptions