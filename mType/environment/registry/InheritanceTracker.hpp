#pragma once
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "../../runtimeTypes/klass/ClassDefinition.hpp"

namespace environment::registry
{
    using namespace runtimeTypes::klass;

    /**
     * @brief Manages class inheritance relationships and hierarchy
     *
     * This class handles all aspects of class inheritance including:
     * - Parent-child relationship tracking
     * - Cycle detection
     * - Inheritance chain computation
     * - Performance optimization through caching
     *
     * Extracted from ClassRegistry to follow Single Responsibility Principle.
     */
    class InheritanceTracker
    {
    private:
        // Relationship maps
        std::unordered_map<std::string, std::vector<std::string>> parentToChildren;
        std::unordered_map<std::string, std::string> childToParent;

        // Performance optimization cache
        mutable std::unordered_map<std::string, std::vector<std::shared_ptr<ClassDefinition>>> inheritanceChainCache;

        // Callback to find class definitions (set by ClassRegistry)
        std::function<std::shared_ptr<ClassDefinition>(const std::string&)> findClassCallback;

        static constexpr int MAX_INHERITANCE_DEPTH = 20;

        // Helper for cycle detection
        bool wouldCreateCycleHelper(const std::string& current, const std::string& target,
                                    std::unordered_set<std::string>& visited, int depth) const;

    public:
        explicit InheritanceTracker() = default;
        ~InheritanceTracker() = default;

        /**
         * @brief Set the callback function to find class definitions
         * @param callback Function that takes class name and returns ClassDefinition
         */
        void setFindClassCallback(std::function<std::shared_ptr<ClassDefinition>(const std::string&)> callback);

        /**
         * @brief Register an inheritance relationship
         * @param childName Name of the child class
         * @param parentName Name of the parent class
         */
        void registerInheritance(const std::string& childName, const std::string& parentName);

        /**
         * @brief Check if registering an inheritance would create a cycle
         * @param childName Name of the child class
         * @param parentName Name of the parent class
         * @return true if it would create a cycle, false otherwise
         */
        [[nodiscard]] bool wouldCreateCycle(const std::string& childName, const std::string& parentName) const;

        /**
         * @brief Get the full inheritance chain for a class
         * @param className Name of the class
         * @return Vector of parent class definitions, from immediate parent to root
         */
        [[nodiscard]] std::vector<std::shared_ptr<ClassDefinition>> getInheritanceChain(const std::string& className) const;

        /**
         * @brief Get all direct children of a parent class
         * @param parentName Name of the parent class
         * @return Vector of child class names
         */
        [[nodiscard]] std::vector<std::string> getChildClasses(const std::string& parentName) const;

        /**
         * @brief Get the parent of a child class
         * @param childName Name of the child class
         * @return Name of parent class, or empty string if no parent
         */
        [[nodiscard]] std::string getParentClass(const std::string& childName) const;

        /**
         * @brief Invalidate cache for a specific class and its descendants
         * @param className Name of the class whose cache should be invalidated
         */
        void invalidateCache(const std::string& className);

        /**
         * @brief Clear all cached inheritance chains
         */
        void clearCache();
    };
}
