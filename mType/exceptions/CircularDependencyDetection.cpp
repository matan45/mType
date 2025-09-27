#include "CircularDependencyDetection.hpp"
#include <algorithm>
#include <unordered_map>
#include <regex>

namespace mtype::exceptions {

    bool DependencyPatternAnalyzer::detectRepeatingPattern(const std::vector<std::string>& chain) const {
        if (chain.size() < config_.repeatingPatternThreshold * 2) {
            return false;
        }

        // Look for repeating subsequences
        for (size_t patternLength = 1; patternLength <= chain.size() / config_.repeatingPatternThreshold; ++patternLength) {
            int repetitions = 0;
            size_t start = chain.size() - patternLength;

            // Check how many times the pattern repeats backwards from the end
            for (size_t pos = start; pos >= patternLength; pos -= patternLength) {
                bool matches = true;
                for (size_t i = 0; i < patternLength; ++i) {
                    if (chain[pos + i] != chain[start + i]) {
                        matches = false;
                        break;
                    }
                }
                if (matches) {
                    repetitions++;
                } else {
                    break;
                }
                if (pos < patternLength) break; // Prevent underflow
            }

            if (repetitions >= config_.repeatingPatternThreshold) {
                return true;
            }
        }

        return false;
    }

    bool DependencyPatternAnalyzer::detectAlternatingPattern(const std::vector<std::string>& chain) const {
        if (chain.size() < config_.alternatingPatternThreshold) {
            return false;
        }

        // Check for A -> B -> A -> B pattern
        int alternations = 0;
        for (size_t i = 2; i < chain.size(); ++i) {
            if (chain[i] == chain[i - 2] && chain[i] != chain[i - 1]) {
                alternations++;
            }
        }

        return alternations >= config_.alternatingPatternThreshold / 2;
    }

    bool DependencyPatternAnalyzer::detectGrowingComplexity(const std::vector<std::string>& chain) const {
        if (chain.size() < 5) {
            return false;
        }

        // Detect if dependency names are getting longer or more complex
        std::vector<size_t> complexities;
        for (const auto& item : chain) {
            // Complexity heuristic: length + number of template brackets + number of namespace separators
            size_t complexity = item.length();
            complexity += std::count(item.begin(), item.end(), '<');
            complexity += std::count(item.begin(), item.end(), '>');
            complexity += std::count(item.begin(), item.end(), ':');
            complexity += std::count(item.begin(), item.end(), ',');
            complexities.push_back(complexity);
        }

        // Check if complexity is consistently growing
        int growthCount = 0;
        for (size_t i = 1; i < complexities.size(); ++i) {
            if (complexities[i] > complexities[i - 1]) {
                growthCount++;
            }
        }

        // If more than 60% of transitions show growth, it's suspicious
        return static_cast<double>(growthCount) / (complexities.size() - 1) > 0.6;
    }

    std::string DependencyPatternAnalyzer::suggestSimplification(const std::vector<std::string>& chain) const {
        if (chain.empty()) {
            return "";
        }

        // Analyze the chain for common issues
        std::string suggestion;

        // Check for repeating patterns
        if (detectRepeatingPattern(chain)) {
            suggestion += "Consider breaking the repeating dependency cycle by introducing an interface or base class. ";
        }

        // Check for alternating patterns
        if (detectAlternatingPattern(chain)) {
            suggestion += "Alternating dependencies detected - consider merging related components or using dependency injection. ";
        }

        // Check for growing complexity
        if (detectGrowingComplexity(chain)) {
            suggestion += "Template complexity is growing - consider simplifying generic type parameters or using type aliases. ";
        }

        // Generic suggestions based on chain length
        if (chain.size() > 20) {
            suggestion += "Very long dependency chain - consider architectural refactoring to reduce coupling. ";
        }

        // Check for obvious cycles in recent elements
        if (chain.size() >= 3) {
            for (size_t i = 0; i < chain.size() - 2; ++i) {
                if (chain[i] == chain.back()) {
                    suggestion += "Direct cycle detected with '" + chain[i] + "' - review interface design. ";
                    break;
                }
            }
        }

        return suggestion.empty() ? "No specific optimization suggestions available." : suggestion;
    }

} // namespace mtype::exceptions