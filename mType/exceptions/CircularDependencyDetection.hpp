#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <chrono>
#include "DomainExceptions.hpp"

namespace mtype::exceptions
{
    /**
     * @brief Types of dependencies that can have circular references
     */
    enum class DependencyType
    {
        GENERIC_SUBSTITUTION,
        IMPORT_CHAIN,
        INTERFACE_INHERITANCE,
        CLASS_INHERITANCE,
        METHOD_OVERLOAD
    };

    /**
     * @brief Configuration for circular dependency detection limits
     */
    struct CircularDependencyConfig
    {
        int maxGenericDepth = 50; // Generic type substitution depth
        int maxImportDepth = 100; // Import chain depth
        int maxInterfaceDepth = 20; // Interface inheritance depth
        int maxClassDepth = 30; // Class inheritance depth
        int maxMethodDepth = 15; // Method overload resolution depth

        bool enableEarlyDetection = true; // Detect patterns before limits
        bool reportDetailedChains = true; // Include full dependency chains
        bool enablePerformanceMetrics = false; // Track detection performance

        // Pattern detection thresholds
        int repeatingPatternThreshold = 3; // How many repetitions indicate a cycle
        int alternatingPatternThreshold = 4; // How many alternations indicate a cycle

        // Performance optimization
        bool enableCaching = true; // Cache validation results
        int cacheMaxSize = 1000; // Maximum cached validations

        /**
         * @brief Get maximum depth for a specific dependency type
         */
        int getMaxDepth(DependencyType type) const
        {
            switch (type)
            {
            case DependencyType::GENERIC_SUBSTITUTION: return maxGenericDepth;
            case DependencyType::IMPORT_CHAIN: return maxImportDepth;
            case DependencyType::INTERFACE_INHERITANCE: return maxInterfaceDepth;
            case DependencyType::CLASS_INHERITANCE: return maxClassDepth;
            case DependencyType::METHOD_OVERLOAD: return maxMethodDepth;
            default: return 50; // Default fallback
            }
        }
    };

    /**
     * @brief Enhanced exception for depth limit violations
     */
    class DepthLimitException : public CircularDependencyException
    {
    private:
        DependencyType type_;
        int currentDepth_;
        int maxDepth_;

    public:
        DepthLimitException(DependencyType type, int current, int max,
                            const std::vector<std::string>& chain,
                            const std::string& location = "")
            : CircularDependencyException(
                  "Depth limit exceeded for " + dependencyTypeToString(type),
                  chain, location)
              , type_(type), currentDepth_(current), maxDepth_(max)
        {
        }

        DependencyType getType() const { return type_; }
        int getCurrentDepth() const { return currentDepth_; }
        int getMaxDepth() const { return maxDepth_; }

        std::string getDetailedMessage() const override
        {
            return "Depth limit exceeded: " + dependencyTypeToString(type_) +
                " depth " + std::to_string(currentDepth_) +
                " exceeds maximum " + std::to_string(maxDepth_) +
                (!location_.empty() ? " at " + location_ : "");
        }

    private:
        static std::string dependencyTypeToString(DependencyType type)
        {
            switch (type)
            {
            case DependencyType::GENERIC_SUBSTITUTION: return "generic substitution";
            case DependencyType::IMPORT_CHAIN: return "import chain";
            case DependencyType::INTERFACE_INHERITANCE: return "interface inheritance";
            case DependencyType::CLASS_INHERITANCE: return "class inheritance";
            case DependencyType::METHOD_OVERLOAD: return "method overload";
            default: return "unknown dependency";
            }
        }
    };

    /**
     * @brief Enhanced exception for true circular dependencies
     */
    class TrueCyclicException : public CircularDependencyException
    {
    private:
        DependencyType type_;
        std::string cycleStart_;

    public:
        TrueCyclicException(DependencyType type, const std::string& cycleStart,
                            const std::vector<std::string>& chain,
                            const std::string& location = "")
            : CircularDependencyException(
                  "True circular dependency detected in " + dependencyTypeToString(type),
                  chain, location)
              , type_(type), cycleStart_(cycleStart)
        {
        }

        DependencyType getType() const { return type_; }
        const std::string& getCycleStart() const { return cycleStart_; }

        std::string getDetailedMessage() const override
        {
            std::string detailed = "Circular dependency detected: " +
                dependencyTypeToString(type_) +
                " cycle starting at '" + cycleStart_ + "'";

            if (!dependencyChain_.empty())
            {
                detailed += "\nDependency chain: ";
                for (size_t i = 0; i < dependencyChain_.size(); ++i)
                {
                    if (i > 0) detailed += " -> ";
                    detailed += dependencyChain_[i];
                }
            }

            if (!location_.empty())
            {
                detailed += "\nAt: " + location_;
            }

            return detailed;
        }

    private:
        static std::string dependencyTypeToString(DependencyType type)
        {
            switch (type)
            {
            case DependencyType::GENERIC_SUBSTITUTION: return "generic substitution";
            case DependencyType::IMPORT_CHAIN: return "import chain";
            case DependencyType::INTERFACE_INHERITANCE: return "interface inheritance";
            case DependencyType::CLASS_INHERITANCE: return "class inheritance";
            case DependencyType::METHOD_OVERLOAD: return "method overload";
            default: return "unknown dependency";
            }
        }
    };

    /**
     * @brief Pattern analyzer for detecting problematic dependency patterns
     */
    class DependencyPatternAnalyzer
    {
    private:
        CircularDependencyConfig config_;

    public:
        explicit DependencyPatternAnalyzer(const CircularDependencyConfig& config)
            : config_(config)
        {
        }

        /**
         * @brief Detect repeating patterns in dependency chain
         * @param chain Current dependency chain
         * @return true if repeating pattern detected
         */
        bool detectRepeatingPattern(const std::vector<std::string>& chain) const;

        /**
         * @brief Detect alternating patterns in dependency chain
         * @param chain Current dependency chain
         * @return true if alternating pattern detected
         */
        bool detectAlternatingPattern(const std::vector<std::string>& chain) const;

        /**
         * @brief Detect growing complexity in dependency chain
         * @param chain Current dependency chain
         * @return true if complexity is growing suspiciously
         */
        bool detectGrowingComplexity(const std::vector<std::string>& chain) const;

        /**
         * @brief Suggest simplification for complex dependency chain
         * @param chain Current dependency chain
         * @return Suggested simplification or empty string
         */
        std::string suggestSimplification(const std::vector<std::string>& chain) const;

        /**
         * @brief Check if any problematic pattern is detected
         * @param chain Current dependency chain
         * @return true if any pattern is detected
         */
        bool detectAnyPattern(const std::vector<std::string>& chain) const
        {
            return detectRepeatingPattern(chain) ||
                detectAlternatingPattern(chain) ||
                detectGrowingComplexity(chain);
        }
    };

    /**
     * @brief Performance metrics for dependency detection
     */
    struct DependencyDetectionMetrics
    {
        std::chrono::milliseconds totalDetectionTime{0};
        int totalDetections = 0;
        int depthLimitViolations = 0;
        int trueCycles = 0;
        int earlyPatternDetections = 0;
        int cacheHits = 0;
        int cacheMisses = 0;

        void addDetection(std::chrono::milliseconds time, bool wasDepthLimit, bool wasTrueCycle, bool wasEarlyPattern)
        {
            totalDetectionTime += time;
            totalDetections++;
            if (wasDepthLimit) depthLimitViolations++;
            if (wasTrueCycle) trueCycles++;
            if (wasEarlyPattern) earlyPatternDetections++;
        }

        void addCacheHit() { cacheHits++; }
        void addCacheMiss() { cacheMisses++; }

        double getAverageDetectionTime() const
        {
            return totalDetections > 0 ? static_cast<double>(totalDetectionTime.count()) / totalDetections : 0.0;
        }

        double getCacheHitRatio() const
        {
            int total = cacheHits + cacheMisses;
            return total > 0 ? static_cast<double>(cacheHits) / total : 0.0;
        }
    };
} // namespace mtype::exceptions
