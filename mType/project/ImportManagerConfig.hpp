#pragma once
#include "ProjectConfig.hpp"
#include "../services/ImportManager.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace project
{
    /**
     * Configure an ImportManager from a ProjectConfig.
     * Shared by ProjectBuilder and DependencyGraphBuilder to avoid duplication.
     *
     * @param importManager        The import manager to configure
     * @param config               The project configuration
     * @param workspaceAliases     Aliases from the workspace (merged with project aliases)
     * @param additionalRoots      Additional allowed roots for workspace member imports
     */
    inline void configureImportManager(
        services::ImportManager& importManager,
        const ProjectConfig& config,
        const std::unordered_map<std::string, std::string>& workspaceAliases = {},
        const std::vector<std::string>& additionalRoots = {})
    {
        importManager.setBaseDirectory(config.projectRoot);
        importManager.setProjectRoot(config.projectRoot);

        for (const auto& root : additionalRoots)
        {
            importManager.addAllowedRoot(root);
        }

        // Merge workspace aliases with project aliases (project takes priority)
        auto merged = workspaceAliases;
        for (const auto& [name, path] : config.imports.aliases)
        {
            merged[name] = path;
        }
        importManager.setPathAliases(merged);
    }
}
