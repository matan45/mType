#pragma once
#include "ProjectConfig.hpp"
#include <string>
#include <vector>
#include <memory>

namespace project
{
    struct MemberProject
    {
        std::string path;
        std::string nameOverride;
        std::unique_ptr<ProjectConfig> config;

        std::string getName() const
        {
            if (!nameOverride.empty())
            {
                return nameOverride;
            }
            if (config)
            {
                return config->name;
            }
            return "";
        }
    };

    struct WorkspaceConfig
    {
        std::string name;
        std::string version;
        std::string workspaceRoot;

        std::vector<MemberProject> members;
        OutputConfig output;
    };
}
