#include "WorkspaceBuilder.hpp"
#include "../services/ScriptInterpreter.hpp"
#include "../services/ImportManager.hpp"
#include <iostream>
#include <fstream>
#include <regex>
#include <queue>
#include <unordered_set>

namespace project
{
    WorkspaceBuilder::WorkspaceBuilder() = default;
    WorkspaceBuilder::~WorkspaceBuilder() = default;

    void WorkspaceBuilder::setProgressCallback(WorkspaceProgressCallback callback)
    {
        progressCallback = std::move(callback);
    }

    WorkspaceBuildResult WorkspaceBuilder::build(const WorkspaceConfig& config)
    {
        WorkspaceBuildResult result;
        auto startTime = std::chrono::steady_clock::now();

        auto buildOrder = computeBuildOrder(config);
        size_t totalProjects = buildOrder.size();

        for (size_t i = 0; i < totalProjects; ++i)
        {
            size_t memberIndex = buildOrder[i];
            const auto& member = config.members[memberIndex];

            auto memberResult = buildMember(member, config, i + 1, totalProjects);

            result.totalFilesCompiled += memberResult.filesCompiled;
            result.totalFilesFailed += memberResult.filesFailed;

            if (memberResult.success)
            {
                ++result.projectsBuilt;
            }
            else
            {
                ++result.projectsFailed;
                result.success = false;

                for (const auto& error : memberResult.errors)
                {
                    result.errors.push_back("[" + member.getName() + "] " + error);
                }
            }
        }

        auto endTime = std::chrono::steady_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        return result;
    }

    void WorkspaceBuilder::clean(const WorkspaceConfig& config)
    {
        ProjectBuilder builder;

        for (const auto& member : config.members)
        {
            if (member.config)
            {
                builder.clean(*member.config);
            }
        }
    }

    std::vector<size_t> WorkspaceBuilder::computeBuildOrder(const WorkspaceConfig& config)
    {
        size_t n = config.members.size();

        // Map project names to member indices
        std::unordered_map<std::string, size_t> nameToIndex;
        for (size_t i = 0; i < n; ++i)
        {
            nameToIndex[config.members[i].getName()] = i;
        }

        // Build adjacency list by scanning source files for @projectName/ imports
        std::vector<std::vector<size_t>> dependsOn(n);
        std::vector<size_t> inDegree(n, 0);

        std::regex importPattern(R"(from\s+"@(\w+)/)");

        for (size_t i = 0; i < n; ++i)
        {
            const auto& member = config.members[i];
            if (!member.config)
            {
                continue;
            }

            std::unordered_set<size_t> deps;
            for (const auto& sourceFile : member.config->resolvedSourceFiles)
            {
                std::ifstream file(sourceFile);
                if (!file.is_open())
                {
                    continue;
                }

                std::string line;
                while (std::getline(file, line))
                {
                    std::smatch match;
                    std::string::const_iterator searchStart = line.cbegin();
                    while (std::regex_search(searchStart, line.cend(), match, importPattern))
                    {
                        std::string depName = match[1].str();
                        auto it = nameToIndex.find(depName);
                        if (it != nameToIndex.end() && it->second != i)
                        {
                            deps.insert(it->second);
                        }
                        searchStart = match.suffix().first;
                    }
                }
            }

            for (size_t dep : deps)
            {
                dependsOn[i].push_back(dep);
                ++inDegree[dep]; // dep is depended upon, but for topo sort we need
                                 // to track incoming edges correctly
            }
        }

        // Kahn's algorithm — build dependencies first
        // Edge direction: if project i depends on project j,
        // j must be built before i. So edges go j -> i.
        // Rebuild with correct edge direction.
        std::vector<std::vector<size_t>> adj(n);
        std::vector<size_t> correctedInDegree(n, 0);

        for (size_t i = 0; i < n; ++i)
        {
            for (size_t dep : dependsOn[i])
            {
                adj[dep].push_back(i);
                ++correctedInDegree[i];
            }
        }

        std::queue<size_t> ready;
        for (size_t i = 0; i < n; ++i)
        {
            if (correctedInDegree[i] == 0)
            {
                ready.push(i);
            }
        }

        std::vector<size_t> order;
        order.reserve(n);

        while (!ready.empty())
        {
            size_t current = ready.front();
            ready.pop();
            order.push_back(current);

            for (size_t next : adj[current])
            {
                if (--correctedInDegree[next] == 0)
                {
                    ready.push(next);
                }
            }
        }

        if (order.size() != n)
        {
            throw std::runtime_error(
                "Circular dependency detected between workspace member projects. "
                "Check cross-project imports for cycles.");
        }

        return order;
    }

    BuildResult WorkspaceBuilder::buildMember(const MemberProject& member,
                                               const WorkspaceConfig& workspace,
                                               size_t projectIndex,
                                               size_t totalProjects)
    {
        if (!member.config)
        {
            BuildResult result;
            result.success = false;
            result.errors.push_back("No project config loaded for member: " + member.path);
            return result;
        }

        // Build workspace aliases so this member can import from siblings
        auto wsAliases = buildWorkspaceAliases(workspace);

        // Collect all member project roots as additional allowed roots
        std::vector<std::string> additionalRoots;
        for (const auto& m : workspace.members)
        {
            if (m.config && m.config->projectRoot != member.config->projectRoot)
            {
                additionalRoots.push_back(m.config->projectRoot);
            }
        }

        ProjectBuilder builder;
        builder.setWorkspaceContext(wsAliases, additionalRoots);

        std::string projectName = member.getName();

        builder.setProgressCallback([this, projectIndex, totalProjects, &projectName]
                                    (const BuildProgress& fileProgress) {
            if (progressCallback)
            {
                WorkspaceBuildProgress wsProgress;
                wsProgress.currentProject = projectIndex;
                wsProgress.totalProjects = totalProjects;
                wsProgress.projectName = projectName;
                wsProgress.fileProgress = fileProgress;
                progressCallback(wsProgress);
            }
        });

        return builder.build(*member.config);
    }

    std::unordered_map<std::string, std::string> WorkspaceBuilder::buildWorkspaceAliases(
        const WorkspaceConfig& config)
    {
        std::unordered_map<std::string, std::string> aliases;

        for (const auto& member : config.members)
        {
            if (member.config)
            {
                std::string aliasName = "@" + member.getName();
                aliases[aliasName] = member.config->projectRoot;
            }
        }

        return aliases;
    }
}
