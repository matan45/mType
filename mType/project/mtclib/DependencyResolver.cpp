#include "DependencyResolver.hpp"
#include <cstddef>
#include <queue>
#include <algorithm>

namespace project::mtclib
{
    DependencyResolver::ResolvedOrder DependencyResolver::resolve(
        const std::unordered_map<std::string, MtcLibProgram>& libraries) const
    {
        ResolvedOrder result;

        if (libraries.empty()) {
            return result;
        }

        // Build adjacency list: library -> its dependencies
        std::unordered_map<std::string, std::vector<std::string>> graph;
        std::unordered_set<std::string> allNodes;

        for (const auto& [name, lib] : libraries) {
            allNodes.insert(name);
            for (const auto& dep : lib.dependencies) {
                graph[name].push_back(dep.name);
                allNodes.insert(dep.name);
            }
            if (graph.find(name) == graph.end()) {
                graph[name] = {};
            }
        }

        // Check for version conflicts
        std::string conflicts = detectVersionConflicts(libraries);
        if (!conflicts.empty()) {
            result.success = false;
            result.errorMessage = conflicts;
            return result;
        }

        // Detect cycles
        auto cycle = detectCycle(graph);
        if (!cycle.empty()) {
            result.success = false;
            result.errorMessage = "Circular library dependency detected: ";
            for (size_t i = 0; i < cycle.size(); ++i) {
                if (i > 0) result.errorMessage += " -> ";
                result.errorMessage += cycle[i];
            }
            return result;
        }

        // Topological sort
        result.loadOrder = topologicalSort(graph, allNodes);
        return result;
    }

    // Helper struct for DFS cycle detection (avoids std::function overhead)
    namespace {
        enum class VisitState { UNVISITED, IN_PROGRESS, DONE };

        bool dfsDetectCycle(
            const std::string& node,
            const std::unordered_map<std::string, std::vector<std::string>>& graph,
            std::unordered_map<std::string, VisitState>& state,
            std::vector<std::string>& path)
        {
            state[node] = VisitState::IN_PROGRESS;
            path.push_back(node);

            auto it = graph.find(node);
            if (it != graph.end()) {
                for (const auto& neighbor : it->second) {
                    if (state.count(neighbor) == 0) continue;  // External dep, not in graph
                    if (state[neighbor] == VisitState::IN_PROGRESS) {
                        // Found cycle — trim path to cycle start
                        auto cycleStart = std::find(path.begin(), path.end(), neighbor);
                        std::vector<std::string> cycle;
                        for (auto iter = cycleStart; iter != path.end(); ++iter) {
                            cycle.push_back(*iter);
                        }
                        cycle.push_back(neighbor);
                        path = cycle;
                        return true;
                    }
                    if (state[neighbor] == VisitState::UNVISITED && dfsDetectCycle(neighbor, graph, state, path)) {
                        return true;
                    }
                }
            }

            path.pop_back();
            state[node] = VisitState::DONE;
            return false;
        }
    }

    std::vector<std::string> DependencyResolver::detectCycle(
        const std::unordered_map<std::string, std::vector<std::string>>& graph) const
    {
        std::unordered_map<std::string, VisitState> state;
        std::vector<std::string> path;

        for (const auto& [node, _] : graph) {
            state[node] = VisitState::UNVISITED;
        }

        for (const auto& [node, _] : graph) {
            if (state[node] == VisitState::UNVISITED) {
                if (dfsDetectCycle(node, graph, state, path)) return path;
            }
        }

        return {};
    }

    std::vector<std::string> DependencyResolver::topologicalSort(
        const std::unordered_map<std::string, std::vector<std::string>>& graph,
        const std::unordered_set<std::string>& allNodes) const
    {
        // Kahn's algorithm
        std::unordered_map<std::string, int> inDegree;
        for (const auto& node : allNodes) {
            inDegree[node] = 0;
        }

        for (const auto& [node, deps] : graph) {
            for (const auto& dep : deps) {
                if (allNodes.count(dep)) {
                    inDegree[dep]++;
                }
            }
        }

        // Note: reverse edges — dependencies should load first
        // So we compute in-degree based on "who depends on me"
        std::unordered_map<std::string, int> revInDegree;
        for (const auto& node : allNodes) {
            revInDegree[node] = 0;
        }
        for (const auto& [node, deps] : graph) {
            revInDegree[node] = static_cast<int>(deps.size());
        }

        std::queue<std::string> ready;
        for (const auto& [node, deg] : revInDegree) {
            if (deg == 0) {
                ready.push(node);
            }
        }

        // Build reverse adjacency: dep -> who depends on dep
        std::unordered_map<std::string, std::vector<std::string>> reverseGraph;
        for (const auto& [node, deps] : graph) {
            for (const auto& dep : deps) {
                reverseGraph[dep].push_back(node);
            }
        }

        std::vector<std::string> order;
        while (!ready.empty()) {
            auto node = ready.front();
            ready.pop();
            order.push_back(node);

            for (const auto& dependent : reverseGraph[node]) {
                revInDegree[dependent]--;
                if (revInDegree[dependent] == 0) {
                    ready.push(dependent);
                }
            }
        }

        return order;
    }

    std::string DependencyResolver::detectVersionConflicts(
        const std::unordered_map<std::string, MtcLibProgram>& libraries) const
    {
        // Track which version of each dependency is required by whom
        // depName -> [(requiredVersion, requiredBy)]
        std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> versionRequirements;

        for (const auto& [libName, lib] : libraries) {
            for (const auto& dep : lib.dependencies) {
                versionRequirements[dep.name].emplace_back(dep.versionConstraint, libName);
            }
        }

        // Check for conflicting versions of the same dependency
        for (const auto& [depName, requirements] : versionRequirements) {
            if (requirements.size() <= 1) continue;

            // Simple conflict detection: if different exact versions are required
            std::unordered_set<std::string> versions;
            for (const auto& [ver, _] : requirements) {
                versions.insert(ver);
            }

            if (versions.size() > 1) {
                std::string error = "Version conflict for library '" + depName + "': ";
                for (size_t i = 0; i < requirements.size(); ++i) {
                    if (i > 0) error += ", ";
                    error += "'" + requirements[i].second + "' requires version " + requirements[i].first;
                }
                return error;
            }
        }

        return "";
    }
}
