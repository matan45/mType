#pragma once

#include <string>
#include <cstdint>

namespace ast
{
    /**
     * @brief Access modifier for class members, methods, and constructors
     *
     * Defines visibility levels for object-oriented access control:
     * - PRIVATE: Accessible only within the same class
     * - PUBLIC: Accessible from anywhere
     * - PROTECTED: Accessible within the same class and subclasses
     */
    enum class AccessModifier : uint8_t
    {
        PRIVATE = 0,    // Default for class members
        PUBLIC = 1,     // Default for interface members
        PROTECTED = 2   // For inheritance-based access
    };

    /**
     * @brief Convert AccessModifier to string representation
     * @param modifier The access modifier to convert
     * @return String representation ("private", "public", "protected")
     */
    const char* accessModifierToString(AccessModifier modifier);

    /**
     * @brief Parse AccessModifier from string
     * @param str String representation of modifier
     * @return Corresponding AccessModifier
     * @throws std::invalid_argument if string is not a valid modifier
     */
    AccessModifier stringToAccessModifier(const std::string& str);

    /**
     * @brief Get default access modifier based on context
     * @param isInterface True if in interface context, false for class
     * @return PRIVATE for classes, PUBLIC for interfaces
     */
    AccessModifier getDefaultAccessModifier(bool isInterface);
}
