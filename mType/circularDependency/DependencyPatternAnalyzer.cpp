#include "DependencyPatternAnalyzer.hpp"
#include <cstddef>
#include <algorithm>
#include <regex>

namespace circularDependency
{
    DependencyPatternAnalyzer::DependencyPatternAnalyzer(const CircularDependencyConfig& config)
        : config_(config)
    {
    }

    bool DependencyPatternAnalyzer::detectAnyPattern(const std::vector<std::string>& chain) const
    {
        return detectRepeatingPattern(chain) ||
            detectAlternatingPattern(chain) ||
            detectGrowingComplexity(chain);
    }

    size_t DependencyPatternAnalyzer::calculateComplexity(const std::string& item) const
    {
        // Complexity heuristic: length + number of template brackets + number of namespace separators
        size_t complexity = item.length();
        complexity += std::count(item.begin(), item.end(), '<');
        complexity += std::count(item.begin(), item.end(), '>');
        complexity += std::count(item.begin(), item.end(), ':');
        complexity += std::count(item.begin(), item.end(), ',');
        return complexity;
    }

    bool DependencyPatternAnalyzer::detectRepeatingPattern(const std::vector<std::string>& chain) const
    {
        if (chain.size() < config_.repeatingPatternThreshold * 2)
        {
            return false;
        }

        // Look for repeating subsequences
        for (size_t patternLength = 1; patternLength <= chain.size() / config_.repeatingPatternThreshold; ++
             patternLength)
        {
            int repetitions = 0;
            size_t start = chain.size() - patternLength;

            // Check how many times the pattern repeats backwards from the end
            for (size_t pos = start; pos >= patternLength; pos -= patternLength)
            {
                bool matches = true;
                for (size_t i = 0; i < patternLength; ++i)
                {
                    if (chain[pos + i] != chain[start + i])
                    {
                        matches = false;
                        break;
                    }
                }
                if (matches)
                {
                    repetitions++;
                }
                else
                {
                    break;
                }
                if (pos < patternLength) break; // Prevent underflow
            }

            if (repetitions >= config_.repeatingPatternThreshold)
            {
                return true;
            }
        }

        return false;
    }

    bool DependencyPatternAnalyzer::detectAlternatingPattern(const std::vector<std::string>& chain) const
    {
        if (chain.size() < config_.alternatingPatternThreshold)
        {
            return false;
        }

        // Check for A -> B -> A -> B pattern
        int alternations = 0;
        for (size_t i = 2; i < chain.size(); ++i)
        {
            if (chain[i] == chain[i - 2] && chain[i] != chain[i - 1])
            {
                alternations++;
            }
        }

        return alternations >= config_.alternatingPatternThreshold / 2;
    }

    bool DependencyPatternAnalyzer::detectGrowingComplexity(const std::vector<std::string>& chain) const
    {
        if (chain.size() < 5)
        {
            return false;
        }

        // Detect if dependency names are getting longer or more complex
        std::vector<size_t> complexities;
        for (const auto& item : chain)
        {
            complexities.push_back(calculateComplexity(item));
        }

        // Check if complexity is consistently growing
        int growthCount = 0;
        for (size_t i = 1; i < complexities.size(); ++i)
        {
            if (complexities[i] > complexities[i - 1])
            {
                growthCount++;
            }
        }

        // If more than threshold of transitions show growth, it's suspicious
        return static_cast<double>(growthCount) / (complexities.size() - 1) > GROWTH_THRESHOLD;
    }

    std::string DependencyPatternAnalyzer::analyzePatternSuggestions(const std::vector<std::string>& chain) const
    {
        std::string suggestion;

        if (detectRepeatingPattern(chain))
        {
            suggestion +=
                "Consider breaking the repeating dependency cycle by introducing an interface or base class. ";
        }

        if (detectAlternatingPattern(chain))
        {
            suggestion +=
                "Alternating dependencies detected - consider merging related components or using dependency injection. ";
        }

        if (detectGrowingComplexity(chain))
        {
            suggestion +=
                "Template complexity is growing - consider simplifying generic type parameters or using type aliases. ";
        }

        return suggestion;
    }

    std::string DependencyPatternAnalyzer::analyzeLengthSuggestions(const std::vector<std::string>& chain) const
    {
        if (chain.size() > LONG_CHAIN_THRESHOLD)
        {
            return "Very long dependency chain - consider architectural refactoring to reduce coupling. ";
        }
        return "";
    }

    std::string DependencyPatternAnalyzer::analyzeCycleSuggestions(const std::vector<std::string>& chain) const
    {
        if (chain.size() < 3)
        {
            return "";
        }

        for (size_t i = 0; i < chain.size() - 2; ++i)
        {
            if (chain[i] == chain.back())
            {
                return "Direct cycle detected with '" + chain[i] + "' - review interface design. ";
            }
        }

        return "";
    }

    std::string DependencyPatternAnalyzer::suggestSimplification(const std::vector<std::string>& chain) const
    {
        if (chain.empty())
        {
            return "";
        }

        std::string suggestion;
        suggestion += analyzePatternSuggestions(chain);
        suggestion += analyzeLengthSuggestions(chain);
        suggestion += analyzeCycleSuggestions(chain);

        return suggestion.empty() ? "No specific optimization suggestions available." : suggestion;
    }
}
