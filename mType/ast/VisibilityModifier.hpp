#pragma once

#include <string>
#include <cstdint>
#include <stdexcept>

namespace ast
{
    /**
     * @brief Visibility modifier for top-level declarations (classes, functions, interfaces)
     *
     * Defines visibility levels for module-level exports:
     * - PUBLIC: Exported and accessible from other files via import
     * - PRIVATE: Not exported, only accessible within the same file
     *
     * Default behavior: PUBLIC (if no modifier is specified)
     *
     * Example usage:
     * ```
     * public class Calculator { }     // Exported
     * private function helper() { }   // Not exported
     * class Foo { }                   // Default: PUBLIC (exported)
     * ```
     */
    enum class VisibilityModifier : uint8_t
    {
        PUBLIC = 0,     // Exported and importable from other files (default)
        PRIVATE = 1     // File-local only, not exported
    };

    /**
     * @brief Convert VisibilityModifier to string representation
     * @param modifier The visibility modifier to convert
     * @return String representation ("public", "private")
     */
    const char* visibilityModifierToString(VisibilityModifier modifier);

    /**
     * @brief Parse VisibilityModifier from string
     * @param str String representation of modifier
     * @return Corresponding VisibilityModifier
     * @throws std::invalid_argument if string is not a valid modifier
     */
    VisibilityModifier stringToVisibilityModifier(const std::string& str);

    /**
     * @brief Get default visibility modifier for top-level declarations
     * @return PUBLIC (default behavior)
     */
    VisibilityModifier getDefaultVisibilityModifier();
}
