#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace project
{
    struct SourceConfig
    {
        std::vector<std::string> include;
        std::vector<std::string> exclude;
    };

    struct OutputConfig
    {
        std::string directory = "build";
    };

    struct ImportsConfig
    {
        std::vector<std::string> searchPaths;
        std::unordered_map<std::string, std::string> aliases;
    };

    struct PackageDependency
    {
        std::string name;
        std::string versionRange;
        std::string source;  // e.g. "github:user/repo", empty = local registry
    };

    struct DependenciesConfig
    {
        std::vector<PackageDependency> packages;
    };

    struct ProjectConfig
    {
        std::string name;
        std::string version;
        std::string projectRoot;

        SourceConfig source;
        OutputConfig output;
        ImportsConfig imports;
        DependenciesConfig dependencies;

        std::vector<std::string> resolvedSourceFiles;
    };
}
