#pragma once
#include <string>
#include <vector>
#include <chrono>
#include "CircularDependencyConfig.hpp"

namespace circularDependency
{
    class DependencyPatternAnalyzer
    {
    private:
        CircularDependencyConfig config_;

    public:
        explicit DependencyPatternAnalyzer(const CircularDependencyConfig& config);

        bool detectAnyPattern(const std::vector<std::string>& chain) const;

        std::string suggestSimplification(const std::vector<std::string>& chain) const;

    private:
        static constexpr double GROWTH_THRESHOLD = 0.6;
        static constexpr size_t LONG_CHAIN_THRESHOLD = 20;

        bool detectRepeatingPattern(const std::vector<std::string>& chain) const;

        bool detectAlternatingPattern(const std::vector<std::string>& chain) const;

        bool detectGrowingComplexity(const std::vector<std::string>& chain) const;

        size_t calculateComplexity(const std::string& item) const;

        std::string analyzePatternSuggestions(const std::vector<std::string>& chain) const;

        std::string analyzeLengthSuggestions(const std::vector<std::string>& chain) const;

        std::string analyzeCycleSuggestions(const std::vector<std::string>& chain) const;
    };
}

