#pragma once
#include <string>
#include <optional>
#include <variant>
#include <filesystem>

namespace project
{
    enum class DiscoveryType
    {
        PROJECT,
        WORKSPACE
    };

    struct DiscoveryResult
    {
        DiscoveryType type;
        std::string path;
    };

    /**
     * Walk up directories from startDir looking for .mtworkspace first,
     * then .mtproj at each level. Returns the first match found.
     */
    inline std::optional<DiscoveryResult> findProjectOrWorkspace(const std::string& startDir = ".")
    {
        std::filesystem::path current = std::filesystem::absolute(startDir);

        while (true)
        {
            std::optional<std::string> workspacePath;
            std::optional<std::string> projectPath;

            for (const auto& entry : std::filesystem::directory_iterator(current))
            {
                if (!entry.is_regular_file())
                {
                    continue;
                }

                std::string filename = entry.path().filename().string();

                if (!workspacePath.has_value() &&
                    filename.size() > 12 &&
                    filename.substr(filename.size() - 12) == ".mtworkspace")
                {
                    workspacePath = entry.path().string();
                }

                if (!projectPath.has_value() &&
                    filename.size() > 7 &&
                    filename.substr(filename.size() - 7) == ".mtproj")
                {
                    projectPath = entry.path().string();
                }
            }

            if (workspacePath.has_value())
            {
                return DiscoveryResult{DiscoveryType::WORKSPACE, workspacePath.value()};
            }

            if (projectPath.has_value())
            {
                return DiscoveryResult{DiscoveryType::PROJECT, projectPath.value()};
            }

            auto parent = current.parent_path();
            if (parent == current)
            {
                break;
            }
            current = parent;
        }

        return std::nullopt;
    }
}
