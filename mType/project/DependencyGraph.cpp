#include "DependencyGraph.hpp"
#include <queue>
#include <algorithm>

namespace project
{
    DependencyGraph::DependencyGraph(
        std::unordered_map<std::string, DependencyNode> nodes,
        std::unordered_map<std::string, std::vector<DependencyEdge>> adjacency,
        std::string projectRoot,
        std::string projectName)
        : nodes_(std::move(nodes))
        , adjacency_(std::move(adjacency))
        , projectRoot_(std::move(projectRoot))
        , projectName_(std::move(projectName))
        , totalEdges_(0)
    {
        buildReverseAdjacency();

        for (const auto& [_, edges] : adjacency_)
        {
            totalEdges_ += edges.size();
        }
    }

    void DependencyGraph::buildReverseAdjacency()
    {
        for (const auto& [source, edges] : adjacency_)
        {
            for (const auto& edge : edges)
            {
                DependencyEdge reverseEdge;
                reverseEdge.from = edge.to;
                reverseEdge.to = edge.from;
                reverseEdge.symbols = edge.symbols;
                reverseEdge.isWildcard = edge.isWildcard;
                reverseAdjacency_[edge.to].push_back(std::move(reverseEdge));
            }
        }
    }

    const std::unordered_map<std::string, DependencyNode>& DependencyGraph::getNodes() const
    {
        return nodes_;
    }

    bool DependencyGraph::hasNode(const std::string& file) const
    {
        return nodes_.find(file) != nodes_.end();
    }

    std::vector<DependencyEdge> DependencyGraph::getDependencies(const std::string& file) const
    {
        auto it = adjacency_.find(file);
        if (it != adjacency_.end())
        {
            return it->second;
        }
        return {};
    }

    std::vector<DependencyEdge> DependencyGraph::getDependents(const std::string& file) const
    {
        auto it = reverseAdjacency_.find(file);
        if (it != reverseAdjacency_.end())
        {
            return it->second;
        }
        return {};
    }

    std::vector<std::string> DependencyGraph::entryPoints() const
    {
        std::vector<std::string> entries;
        for (const auto& [path, _] : nodes_)
        {
            if (reverseAdjacency_.find(path) == reverseAdjacency_.end())
            {
                entries.push_back(path);
            }
        }
        std::sort(entries.begin(), entries.end());
        return entries;
    }

    // Kahn's algorithm
    std::vector<std::string> DependencyGraph::topologicalOrder() const
    {
        std::unordered_map<std::string, int> inDegree;
        for (const auto& [path, _] : nodes_)
        {
            inDegree[path] = 0;
        }

        for (const auto& [_, edges] : adjacency_)
        {
            for (const auto& edge : edges)
            {
                inDegree[edge.to]++;
            }
        }

        std::queue<std::string> ready;
        for (const auto& [path, degree] : inDegree)
        {
            if (degree == 0)
            {
                ready.push(path);
            }
        }

        std::vector<std::string> order;
        order.reserve(nodes_.size());

        while (!ready.empty())
        {
            std::string current = ready.front();
            ready.pop();
            order.push_back(current);

            auto it = adjacency_.find(current);
            if (it != adjacency_.end())
            {
                for (const auto& edge : it->second)
                {
                    if (--inDegree[edge.to] == 0)
                    {
                        ready.push(edge.to);
                    }
                }
            }
        }

        // If order is incomplete, graph has cycles
        if (order.size() != nodes_.size())
        {
            return {};
        }
        return order;
    }

    // Tarjan's SCC — finds all strongly connected components with size > 1
    void DependencyGraph::strongConnect(
        const std::string& node,
        int& index,
        std::unordered_map<std::string, int>& nodeIndex,
        std::unordered_map<std::string, int>& lowLink,
        std::unordered_map<std::string, bool>& onStack,
        std::vector<std::string>& stack,
        std::vector<std::vector<std::string>>& sccs) const
    {
        nodeIndex[node] = index;
        lowLink[node] = index;
        index++;
        stack.push_back(node);
        onStack[node] = true;

        auto it = adjacency_.find(node);
        if (it != adjacency_.end())
        {
            for (const auto& edge : it->second)
            {
                if (nodeIndex.find(edge.to) == nodeIndex.end())
                {
                    strongConnect(edge.to, index, nodeIndex, lowLink,
                                  onStack, stack, sccs);
                    lowLink[node] = std::min(lowLink[node], lowLink[edge.to]);
                }
                else if (onStack[edge.to])
                {
                    lowLink[node] = std::min(lowLink[node], nodeIndex[edge.to]);
                }
            }
        }

        if (lowLink[node] == nodeIndex[node])
        {
            std::vector<std::string> scc;
            std::string w;
            do
            {
                w = stack.back();
                stack.pop_back();
                onStack[w] = false;
                scc.push_back(w);
            } while (w != node);

            // Only report cycles (SCC with more than 1 node)
            if (scc.size() > 1)
            {
                std::reverse(scc.begin(), scc.end());
                sccs.push_back(std::move(scc));
            }
        }
    }

    std::vector<std::vector<std::string>> DependencyGraph::findCycles() const
    {
        int index = 0;
        std::unordered_map<std::string, int> nodeIndex;
        std::unordered_map<std::string, int> lowLink;
        std::unordered_map<std::string, bool> onStack;
        std::vector<std::string> stack;
        std::vector<std::vector<std::string>> sccs;

        for (const auto& [path, _] : nodes_)
        {
            if (nodeIndex.find(path) == nodeIndex.end())
            {
                strongConnect(path, index, nodeIndex, lowLink,
                              onStack, stack, sccs);
            }
        }

        return sccs;
    }

    // BFS shortest path
    std::vector<std::string> DependencyGraph::findPath(
        const std::string& source, const std::string& target) const
    {
        if (!hasNode(source) || !hasNode(target))
        {
            return {};
        }

        if (source == target)
        {
            return {source};
        }

        std::queue<std::string> queue;
        std::unordered_map<std::string, std::string> predecessor;
        std::unordered_set<std::string> visited;

        queue.push(source);
        visited.insert(source);

        while (!queue.empty())
        {
            std::string current = queue.front();
            queue.pop();

            auto it = adjacency_.find(current);
            if (it == adjacency_.end()) continue;

            for (const auto& edge : it->second)
            {
                if (visited.count(edge.to)) continue;

                predecessor[edge.to] = current;
                if (edge.to == target)
                {
                    // Reconstruct path
                    std::vector<std::string> path;
                    std::string node = target;
                    while (node != source)
                    {
                        path.push_back(node);
                        node = predecessor[node];
                    }
                    path.push_back(source);
                    std::reverse(path.begin(), path.end());
                    return path;
                }

                visited.insert(edge.to);
                queue.push(edge.to);
            }
        }

        return {};
    }

    // DFS transitive closure
    std::unordered_set<std::string> DependencyGraph::transitiveDependencies(
        const std::string& file) const
    {
        std::unordered_set<std::string> result;
        std::vector<std::string> stack;
        stack.push_back(file);

        while (!stack.empty())
        {
            std::string current = stack.back();
            stack.pop_back();

            auto it = adjacency_.find(current);
            if (it == adjacency_.end()) continue;

            for (const auto& edge : it->second)
            {
                if (result.insert(edge.to).second)
                {
                    stack.push_back(edge.to);
                }
            }
        }

        return result;
    }

    size_t DependencyGraph::nodeCount() const
    {
        return nodes_.size();
    }

    size_t DependencyGraph::edgeCount() const
    {
        return totalEdges_;
    }

    const std::string& DependencyGraph::getProjectRoot() const
    {
        return projectRoot_;
    }

    const std::string& DependencyGraph::getProjectName() const
    {
        return projectName_;
    }
}
