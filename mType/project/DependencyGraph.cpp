#include "DependencyGraph.hpp"
#include <queue>
#include <algorithm>
#include <stack>

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

        // If order is incomplete, graph has cycles — return empty
        if (order.size() != nodes_.size())
        {
            return {};
        }
        return order;
    }

    // Iterative Tarjan's SCC — avoids stack overflow on deep graphs
    std::vector<std::vector<std::string>> DependencyGraph::findCycles() const
    {
        int index = 0;
        std::unordered_map<std::string, int> nodeIndex;
        std::unordered_map<std::string, int> lowLink;
        std::unordered_map<std::string, bool> onStack;
        std::vector<std::string> sccStack;
        std::vector<std::vector<std::string>> sccs;

        // Iterative DFS frame
        struct Frame
        {
            std::string node;
            size_t edgeIdx;  // which neighbour to visit next
        };

        for (const auto& [startNode, _] : nodes_)
        {
            if (nodeIndex.count(startNode)) continue;

            std::stack<Frame> dfs;
            dfs.push({startNode, 0});
            nodeIndex[startNode] = index;
            lowLink[startNode] = index;
            index++;
            sccStack.push_back(startNode);
            onStack[startNode] = true;

            while (!dfs.empty())
            {
                auto& frame = dfs.top();
                const std::string& node = frame.node;

                auto adjIt = adjacency_.find(node);
                const std::vector<DependencyEdge>* edges = nullptr;
                if (adjIt != adjacency_.end())
                {
                    edges = &adjIt->second;
                }

                bool pushed = false;
                while (edges && frame.edgeIdx < edges->size())
                {
                    const auto& neighbour = (*edges)[frame.edgeIdx].to;
                    frame.edgeIdx++;

                    if (!nodeIndex.count(neighbour))
                    {
                        nodeIndex[neighbour] = index;
                        lowLink[neighbour] = index;
                        index++;
                        sccStack.push_back(neighbour);
                        onStack[neighbour] = true;
                        dfs.push({neighbour, 0});
                        pushed = true;
                        break;
                    }
                    else if (onStack[neighbour])
                    {
                        lowLink[node] = std::min(lowLink[node],
                                                  nodeIndex[neighbour]);
                    }
                }

                if (pushed) continue;

                // All neighbours explored — save node before pop
                std::string currentNode = node;

                if (lowLink[currentNode] == nodeIndex[currentNode])
                {
                    std::vector<std::string> scc;
                    std::string w;
                    do
                    {
                        w = sccStack.back();
                        sccStack.pop_back();
                        onStack[w] = false;
                        scc.push_back(w);
                    } while (w != currentNode);

                    if (scc.size() > 1)
                    {
                        std::reverse(scc.begin(), scc.end());
                        sccs.push_back(std::move(scc));
                    }
                }

                dfs.pop();

                // Update parent's lowLink using saved copy
                if (!dfs.empty())
                {
                    lowLink[dfs.top().node] = std::min(
                        lowLink[dfs.top().node], lowLink[currentNode]);
                }
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
