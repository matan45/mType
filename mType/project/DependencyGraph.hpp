#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

namespace project
{
    enum class NodeKind : uint8_t
    {
        SOURCE_FILE,    // .mt file inside the project
        EXTERNAL_FILE   // .mt file outside the project (library import)
    };

    struct DependencyNode
    {
        std::string filePath;      // canonical absolute path
        std::string relativePath;  // relative to project root (for display)
        NodeKind kind;
    };

    struct DependencyEdge
    {
        std::string from;                  // canonical path of importer
        std::string to;                    // canonical path of importee
        std::vector<std::string> symbols;  // empty for wildcard imports
        bool isWildcard;
    };

    /**
     * Immutable file-level dependency graph.
     * Built once per invocation by DependencyGraphBuilder.
     */
    class DependencyGraph
    {
    public:
        DependencyGraph(
            std::unordered_map<std::string, DependencyNode> nodes,
            std::unordered_map<std::string, std::vector<DependencyEdge>> adjacency,
            std::string projectRoot,
            std::string projectName);

        // Node queries
        const std::unordered_map<std::string, DependencyNode>& getNodes() const;
        bool hasNode(const std::string& file) const;

        // Edge queries
        std::vector<DependencyEdge> getDependencies(const std::string& file) const;
        std::vector<DependencyEdge> getDependents(const std::string& file) const;

        // Graph algorithms
        std::vector<std::string> topologicalOrder() const;
        std::vector<std::vector<std::string>> findCycles() const;
        std::vector<std::string> findPath(
            const std::string& source, const std::string& target) const;
        std::unordered_set<std::string> transitiveDependencies(
            const std::string& file) const;

        // Entry points: nodes with in-degree 0
        std::vector<std::string> entryPoints() const;

        // Stats
        size_t nodeCount() const;
        size_t edgeCount() const;
        const std::string& getProjectRoot() const;
        const std::string& getProjectName() const;

    private:
        std::unordered_map<std::string, DependencyNode> nodes_;
        std::unordered_map<std::string, std::vector<DependencyEdge>> adjacency_;
        std::unordered_map<std::string, std::vector<DependencyEdge>> reverseAdjacency_;
        std::string projectRoot_;
        std::string projectName_;
        size_t totalEdges_;

        void buildReverseAdjacency();

        // Tarjan's SCC helper
        void strongConnect(
            const std::string& node,
            int& index,
            std::unordered_map<std::string, int>& nodeIndex,
            std::unordered_map<std::string, int>& lowLink,
            std::unordered_map<std::string, bool>& onStack,
            std::vector<std::string>& stack,
            std::vector<std::vector<std::string>>& sccs) const;
    };
}
