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
        
        bool detectRepeatingPattern(const std::vector<std::string>& chain) const;
        
        bool detectAlternatingPattern(const std::vector<std::string>& chain) const;
        
        bool detectGrowingComplexity(const std::vector<std::string>& chain) const;
        
        std::string suggestSimplification(const std::vector<std::string>& chain) const;

       
        bool detectAnyPattern(const std::vector<std::string>& chain) const
        {
            return detectRepeatingPattern(chain) ||
                detectAlternatingPattern(chain) ||
                detectGrowingComplexity(chain);
        }
    };
}

