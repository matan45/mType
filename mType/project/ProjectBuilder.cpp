#include "ProjectBuilder.hpp"
#include "../services/ScriptInterpreter.hpp"
#include "../services/ImportManager.hpp"
#include <iostream>
#include <filesystem>

namespace project
{
    ProjectBuilder::ProjectBuilder() = default;
    ProjectBuilder::~ProjectBuilder() = default;

    void ProjectBuilder::setProgressCallback(ProgressCallback callback)
    {
        progressCallback = std::move(callback);
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

    bool ProjectBuilder::compileFile(const std::string& sourcePath,
                                      const std::string& outputPath,
                                      const ProjectConfig& config)
    {
        try
        {
            services::ScriptInterpreter interpreter;

            // Configure search paths relative to project root
            std::vector<std::string> absoluteSearchPaths;
            for (const auto& searchPath : config.imports.searchPaths)
            {
                std::filesystem::path absPath = std::filesystem::path(config.projectRoot) / searchPath;
                absoluteSearchPaths.push_back(absPath.string());
            }

            interpreter.compileToFile(sourcePath, outputPath,
                                      absoluteSearchPaths,
                                      config.imports.aliases);
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error compiling " << sourcePath << ": " << e.what() << std::endl;
            return false;
        }
    }

    void ProjectBuilder::ensureOutputDirectories(const ProjectConfig& config,
                                                  const std::vector<std::string>& sourceFiles)
    {
        std::filesystem::path outputBase = std::filesystem::path(config.projectRoot) / config.output.directory;

        if (!std::filesystem::exists(outputBase))
        {
            std::filesystem::create_directories(outputBase);
        }

        for (const auto& sourceFile : sourceFiles)
        {
            std::string outputPath = getOutputPath(sourceFile, config);
            std::filesystem::path outputDir = std::filesystem::path(outputPath).parent_path();

            if (!std::filesystem::exists(outputDir))
            {
                std::filesystem::create_directories(outputDir);
            }
        }
    }

    std::string ProjectBuilder::getOutputPath(const std::string& sourcePath,
                                               const ProjectConfig& config)
    {
        std::filesystem::path source(sourcePath);
        std::filesystem::path projectRoot(config.projectRoot);
        std::filesystem::path outputBase = projectRoot / config.output.directory;

        std::filesystem::path relativePath = std::filesystem::relative(source, projectRoot);

        std::filesystem::path outputPath = outputBase / relativePath;

        std::string outputStr = outputPath.string();
        if (outputStr.size() >= 3 && outputStr.substr(outputStr.size() - 3) == ".mt")
        {
            outputStr = outputStr.substr(0, outputStr.size() - 3) + ".mtc";
        }
        else
        {
            outputStr += ".mtc";
        }

        return outputStr;
    }

    void ProjectBuilder::configureImportManager(services::ImportManager& importManager,
                                                 const ProjectConfig& config)
    {
        importManager.setBaseDirectory(config.projectRoot);
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
