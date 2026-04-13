#pragma once
#include "MtcLibFormat.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace project::mtclib
{
    /**
     * Resolves library dependency ordering via topological sort.
     * Detects circular dependencies and diamond version conflicts.
     */
    class DependencyResolver
    {
    public:
        struct ResolvedOrder
        {
            std::vector<std::string> loadOrder;  // Topologically sorted library names
            bool success = true;
            std::string errorMessage;
        };

        // Perform topological sort on the dependency graph
        ResolvedOrder resolve(const std::unordered_map<std::string, MtcLibProgram>& libraries) const;

    private:
        // Detect circular dependencies (returns cycle path or empty)
        std::vector<std::string> detectCycle(
            const std::unordered_map<std::string, std::vector<std::string>>& graph) const;

        // Kahn's algorithm for topological sort
        std::vector<std::string> topologicalSort(
            const std::unordered_map<std::string, std::vector<std::string>>& graph,
            const std::unordered_set<std::string>& allNodes) const;

        // Detect diamond version conflicts
        std::string detectVersionConflicts(
            const std::unordered_map<std::string, MtcLibProgram>& libraries) const;
    };
}
