#pragma once

#include <string>
#include <vector>
#include "../errors/SourceLocation.hpp"

namespace ast
{
    /**
     * Represents a generic type parameter in class or method declarations.
     * For example, the 'T' in 'class Box<T>' or the 'E' in 'function <E> test(E value)'.
     */
    struct GenericTypeParameter
    {
        std::string name;                    // e.g., "T", "E", "K", "V"
        std::vector<std::string> constraints; // Future: for bounded generics like "T extends SomeClass"
        errors::SourceLocation location;    // Source location for error reporting

        /**
         * Constructor for a simple generic type parameter without constraints.
         * @param paramName The name of the type parameter (e.g., "T")
         * @param loc Source location where this parameter is defined
         */
        explicit GenericTypeParameter(const std::string& paramName,
                                    const errors::SourceLocation& loc = errors::SourceLocation())
            : name(paramName), location(loc) {}

        /**
         * Constructor for a generic type parameter with constraints.
         * @param paramName The name of the type parameter (e.g., "T")
         * @param bounds Vector of constraint class names (e.g., {"Comparable", "Serializable"})
         * @param loc Source location where this parameter is defined
         */
        GenericTypeParameter(const std::string& paramName,
                           const std::vector<std::string>& bounds,
                           const errors::SourceLocation& loc = errors::SourceLocation())
            : name(paramName), constraints(bounds), location(loc) {}

        /**
         * Checks if this type parameter has any constraints.
         * @return true if constraints are defined, false otherwise
         */
        bool hasConstraints() const { return !constraints.empty(); }

        /**
         * Gets a string representation of this type parameter.
         * @return String like "T" or "T extends Comparable"
         */
        std::string toString() const;

        /**
         * Equality operator for comparing type parameters.
         * @param other The other parameter to compare with
         * @return true if names match (constraints are ignored in comparison)
         */
        bool operator==(const GenericTypeParameter& other) const {
            return name == other.name;
        }

        /**
         * Inequality operator.
         * @param other The other parameter to compare with
         * @return true if names don't match
         */
        bool operator!=(const GenericTypeParameter& other) const {
            return !(*this == other);
        }
    };
}