#pragma once
#include "DependencyType.hpp"

namespace circularDependency
{
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
}
