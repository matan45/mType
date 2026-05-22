#pragma once
#include "ProjectConfig.hpp"
#include "MtModulesManager.hpp"
#include "../services/ImportManager.hpp"
#include <exception>
#include <filesystem>
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
        namespace fs = std::filesystem;

        importManager.setBaseDirectory(config.projectRoot);
        importManager.setProjectRoot(config.projectRoot);

        for (const auto& root : additionalRoots)
        {
            importManager.addAllowedRoot(root);
        }

        std::vector<std::string> absoluteSearchPaths;
        absoluteSearchPaths.reserve(config.imports.searchPaths.size());
        for (const auto& searchPath : config.imports.searchPaths)
        {
            fs::path absPath = fs::path(config.projectRoot) / searchPath;
            absoluteSearchPaths.push_back(absPath.string());
            importManager.addAllowedRoot(absPath.string());
        }
        importManager.setSearchPaths(absoluteSearchPaths);

        // Match ProjectBuilder::buildMergedAliases precedence:
        // workspace aliases < mt_modules aliases < explicit project aliases.
        auto merged = workspaceAliases;
        try
        {
            packagemanager::MtModulesManager modulesMgr(config.projectRoot);
            for (const auto& [name, path] : modulesMgr.getAliases())
            {
                merged[name] = path;
            }
        }
        catch (const std::exception&)
        {
            // A broken mt_modules/ must not block projects that do not use it.
        }

        for (const auto& [name, path] : config.imports.aliases)
        {
            merged[name] = path;
        }
        importManager.setPathAliases(merged);

        for (const auto& [_, path] : merged)
        {
            if (path.empty()) continue;

            fs::path aliasRoot(path);
            if (aliasRoot.is_relative())
            {
                aliasRoot = fs::path(config.projectRoot) / aliasRoot;
            }
            importManager.addAllowedRoot(aliasRoot.lexically_normal().string());
        }
    }
}
