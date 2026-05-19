#include "ProjectBuilder.hpp"
#include <cstddef>
#include <filesystem>

namespace project
{
    ProjectBuilder::ProjectBuilder() = default;
    ProjectBuilder::~ProjectBuilder() = default;

    void ProjectBuilder::setProgressCallback(ProgressCallback callback)
    {
        progressCallback = std::move(callback);
    }

    void ProjectBuilder::setWorkspaceContext(
        const std::unordered_map<std::string, std::string>& aliases,
        const std::vector<std::string>& additionalRoots)
    {
        workspaceAliases = aliases;
        workspaceAdditionalRoots = additionalRoots;
    }

    BuildResult ProjectBuilder::build(const ProjectConfig& config)
    {
        BuildResult result;
        auto startTime = std::chrono::steady_clock::now();

        const auto& sourceFiles = config.resolvedSourceFiles;
        size_t total = sourceFiles.size();

        if (total == 0)
        {
            result.errors.push_back("No source files to compile");
            result.success = false;
            auto endTime = std::chrono::steady_clock::now();
            result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            return result;
        }

        ensureOutputDirectories(config, sourceFiles);

        for (size_t i = 0; i < total; ++i)
        {
            const auto& sourceFile = sourceFiles[i];
            std::string outputPath = getOutputPath(sourceFile, config);

            reportProgress(i + 1, total, sourceFile);

            try
            {
                if (compileFile(sourceFile, outputPath, config))
                {
                    ++result.filesCompiled;
                }
                else
                {
                    ++result.filesFailed;
                    result.success = false;
                }
            }
            catch (const std::exception& e)
            {
                ++result.filesFailed;
                result.success = false;
                result.errors.push_back(sourceFile + ": " + e.what());
            }
        }

        auto endTime = std::chrono::steady_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        return result;
    }

    void ProjectBuilder::clean(const ProjectConfig& config)
    {
        std::filesystem::path outputDir = std::filesystem::path(config.projectRoot) / config.output.directory;

        if (std::filesystem::exists(outputDir))
        {
            std::filesystem::remove_all(outputDir);
        }
    }

    void ProjectBuilder::reportProgress(size_t current, size_t total, const std::string& file)
    {
        if (progressCallback)
        {
            BuildProgress progress;
            progress.current = current;
            progress.total = total;
            progress.currentFile = file;
            progressCallback(progress);
        }
    }
}
