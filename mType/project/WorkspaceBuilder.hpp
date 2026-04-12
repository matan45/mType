#pragma once
#include "WorkspaceConfig.hpp"
#include "ProjectBuilder.hpp"
#include <functional>
#include <chrono>
#include <vector>
#include <unordered_map>

namespace project
{
    struct WorkspaceBuildResult
    {
        bool success = true;
        size_t projectsBuilt = 0;
        size_t projectsFailed = 0;
        size_t totalFilesCompiled = 0;
        size_t totalFilesFailed = 0;
        std::vector<std::string> errors;
        std::chrono::milliseconds duration{0};
    };

    struct WorkspaceBuildProgress
    {
        size_t currentProject;
        size_t totalProjects;
        std::string projectName;
        BuildProgress fileProgress;
    };

    using WorkspaceProgressCallback = std::function<void(const WorkspaceBuildProgress&)>;

    class WorkspaceBuilder
    {
    public:
        WorkspaceBuilder();
        ~WorkspaceBuilder();

        void setProgressCallback(WorkspaceProgressCallback callback);

        WorkspaceBuildResult build(const WorkspaceConfig& config);

        void clean(const WorkspaceConfig& config);

    private:
        WorkspaceProgressCallback progressCallback;

        std::vector<size_t> computeBuildOrder(const WorkspaceConfig& config);

        BuildResult buildMember(const MemberProject& member,
                                const WorkspaceConfig& workspace,
                                size_t projectIndex,
                                size_t totalProjects);

        std::unordered_map<std::string, std::string> buildWorkspaceAliases(
            const WorkspaceConfig& config);
    };
}
