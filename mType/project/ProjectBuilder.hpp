#pragma once
#include "ProjectConfig.hpp"
#include <functional>
#include <chrono>
#include <vector>
#include <memory>
#include <unordered_map>

namespace environment
{
    class Environment;
}

namespace services
{
    class ScriptInterpreter;
    class ImportManager;
}

namespace project
{
    struct BuildResult
    {
        bool success = true;
        size_t filesCompiled = 0;
        size_t filesFailed = 0;
        std::vector<std::string> errors;
        std::chrono::milliseconds duration{0};
    };

    struct BuildProgress
    {
        size_t current;
        size_t total;
        std::string currentFile;
    };

    using ProgressCallback = std::function<void(const BuildProgress&)>;

    class ProjectBuilder
    {
    public:
        ProjectBuilder();
        ~ProjectBuilder();

        void setProgressCallback(ProgressCallback callback);

        // Set workspace context for cross-project imports.
        // aliases: map of @projectName -> absolute project root path
        // additionalRoots: allowed roots for security containment
        void setWorkspaceContext(
            const std::unordered_map<std::string, std::string>& aliases,
            const std::vector<std::string>& additionalRoots);

        BuildResult build(const ProjectConfig& config);

        BuildResult buildLibrary(const ProjectConfig& config, const std::string& outputPath);

        // Build library using a pre-configured environment (with native functions registered)
        BuildResult buildLibrary(const ProjectConfig& config, const std::string& outputPath,
                                 std::shared_ptr<environment::Environment> env);

        void clean(const ProjectConfig& config);

    private:
        ProgressCallback progressCallback;
        std::unordered_map<std::string, std::string> workspaceAliases;
        std::vector<std::string> workspaceAdditionalRoots;

        bool compileFile(const std::string& sourcePath,
                         const std::string& outputPath,
                         const ProjectConfig& config);

        void ensureOutputDirectories(const ProjectConfig& config,
                                     const std::vector<std::string>& sourceFiles);

        std::string getOutputPath(const std::string& sourcePath,
                                  const ProjectConfig& config);

        void configureImportManager(services::ImportManager& importManager,
                                    const ProjectConfig& config);

        void reportProgress(size_t current, size_t total, const std::string& file);
    };
}
