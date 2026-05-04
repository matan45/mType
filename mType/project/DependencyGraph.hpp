#pragma once
#include <string>
#include <cstddef>
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
        NodeKind kind = NodeKind::SOURCE_FILE;
    };

    struct DependencyEdge
    {
        std::string from;                  // canonical path of importer
        std::string to;                    // canonical path of importee
        std::vector<std::string> symbols;  // empty for wildcard imports
        bool isWildcard = false;
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

        // Returns files in topological order (Kahn's algorithm).
        // Returns an EMPTY vector when the graph contains cycles — callers
        // should use findCycles() to distinguish "no files" from "has cycles".
        [[nodiscard]] std::vector<std::string> topologicalOrder() const;

        [[nodiscard]] std::vector<std::vector<std::string>> findCycles() const;

        [[nodiscard]] std::vector<std::string> findPath(
            const std::string& source, const std::string& target) const;

        [[nodiscard]] std::unordered_set<std::string> transitiveDependencies(
            const std::string& file) const;

        // Entry points: nodes with in-degree 0
        [[nodiscard]] std::vector<std::string> entryPoints() const;

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
    };
}
