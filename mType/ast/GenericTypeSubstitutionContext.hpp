#pragma once
#include "../circularDependency/CircularDependencyDetector.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ast
{
    /**
     * Context for tracking generic type substitutions and detecting circular dependencies.
     * Used during generic type parameter substitution to prevent infinite recursion.
     */
    struct GenericTypeSubstitutionContext
    {
        std::shared_ptr<circularDependency::CircularDependencyDetector> detector;
        std::string currentLocation; // For error reporting

        GenericTypeSubstitutionContext();

        explicit GenericTypeSubstitutionContext(const circularDependency::CircularDependencyConfig& config);

        /**
         * @brief Enter a new substitution step with enhanced detection
         * @param paramName Parameter being substituted
         * @param location Optional source location for error reporting
         * @return true if safe to proceed, throws exception if circular dependency detected
         */
        bool enterSubstitution(const std::string& paramName, const std::string& location = "");

        /**
         * @brief Exit current substitution step
         * @param paramName Parameter being exited
         */
        void exitSubstitution(const std::string& paramName);

        /**
         * @brief Get current substitution chain
         * @return Current dependency chain
         */
        std::vector<std::string> getCurrentChain() const;

        /**
         * @brief Get current substitution depth
         * @return Current depth
         */
        int getCurrentDepth() const;

        /**
         * @brief Get current substitution path for error reporting
         * @return String representation of substitution chain
         */
        std::string getChainString() const;
    };
}
