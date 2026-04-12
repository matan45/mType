#include "ProjectBuilder.hpp"
#include "../services/ScriptInterpreter.hpp"
#include "../services/ImportManager.hpp"
#include "../lexer/Lexer.hpp"
#include "../parser/Parser.hpp"
#include "../vm/compiler/BytecodeCompiler.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../services/OptimizationService.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>

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

    BuildResult ProjectBuilder::buildLibrary(const ProjectConfig& config, const std::string& outputPath)
    {
        // Use default environment (no native functions)
        environment::EnvironmentBuilder envBuilder;
        auto environment = envBuilder.build();
        return buildLibrary(config, outputPath, environment);
    }

    BuildResult ProjectBuilder::buildLibrary(const ProjectConfig& config, const std::string& outputPath,
                                              std::shared_ptr<environment::Environment> environment)
    {
        BuildResult result;
        auto startTime = std::chrono::steady_clock::now();

        const auto& sourceFiles = config.resolvedSourceFiles;

        if (sourceFiles.empty())
        {
            result.errors.push_back("No source files to compile");
            result.success = false;
            auto endTime = std::chrono::steady_clock::now();
            result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            return result;
        }

        try
        {
            // Create a virtual source that imports all files
            std::string virtualSource;
            for (const auto& sourceFile : sourceFiles)
            {
                // Use relative path from project root
                std::filesystem::path srcPath(sourceFile);
                std::filesystem::path projRoot(config.projectRoot);
                std::filesystem::path relativePath = std::filesystem::relative(srcPath, projRoot);
                virtualSource += "import * from \"" + relativePath.string() + "\";\n";
            }

            // Create temp file for virtual source
            std::filesystem::path tempDir = std::filesystem::temp_directory_path();
            std::filesystem::path tempFile = tempDir / "_mtype_lib_bundle.mt";

            {
                std::ofstream out(tempFile);
                out << virtualSource;
            }

            reportProgress(1, 1, "Building library...");

            // Configure search paths
            std::vector<std::string> absoluteSearchPaths;
            for (const auto& searchPath : config.imports.searchPaths)
            {
                std::filesystem::path absPath = std::filesystem::path(config.projectRoot) / searchPath;
                absoluteSearchPaths.push_back(absPath.string());
            }

            // Create import manager with project settings
            auto importManager = std::make_unique<services::ImportManager>();
            importManager->setBaseDirectory(config.projectRoot);
            importManager->setProjectRoot(config.projectRoot); // SECURITY: enforce containment

            // Add workspace-level allowed roots for cross-project imports
            for (const auto& root : workspaceAdditionalRoots)
            {
                importManager->addAllowedRoot(root);
            }

            importManager->setSearchPaths(absoluteSearchPaths);
            importManager->setPathAliases(buildMergedAliases(config));
            importManager->setCurrentFilePath(tempFile.string());

            services::ImportManager* importMgrPtr = importManager.get();

            // Parse the virtual source
            lexer::Lexer lexer(tempFile.string());
            parser::Parser parser(lexer, std::move(importManager));
            auto ast = parser.parseProgram();

            // Set ImportManager on environment
            environment->setImportManager(importMgrPtr);

            // Resolve imports
            importMgrPtr->resolveAllImports(ast.get());

            // Apply optimizations
            services::OptimizationService optimizationService;
            ast = optimizationService.applyOptimizations(std::move(ast), environment);

            // Compile to bytecode
            vm::compiler::BytecodeCompiler compiler(environment, true);
            auto program = compiler.compile(ast.get());

            // Ensure output directory exists
            std::filesystem::path outPath(outputPath);
            if (outPath.has_parent_path() && !std::filesystem::exists(outPath.parent_path()))
            {
                std::filesystem::create_directories(outPath.parent_path());
            }

            // Serialize to library file
            std::ofstream outFile(outputPath, std::ios::binary);
            if (!outFile)
            {
                throw std::runtime_error("Could not create output file: " + outputPath);
            }
            program.serialize(outFile);
            outFile.close();

            // Clean up temp file
            std::filesystem::remove(tempFile);

            result.filesCompiled = sourceFiles.size();
        }
        catch (const std::exception& e)
        {
            result.success = false;
            result.errors.push_back(e.what());
        }

        auto endTime = std::chrono::steady_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        return result;
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
                                      buildMergedAliases(config));
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

    std::unordered_map<std::string, std::string> ProjectBuilder::buildMergedAliases(
        const ProjectConfig& config) const
    {
        auto merged = workspaceAliases;
        for (const auto& [name, path] : config.imports.aliases)
        {
            merged[name] = path;
        }
        return merged;
    }

    void ProjectBuilder::configureImportManager(services::ImportManager& importManager,
                                                 const ProjectConfig& config)
    {
        importManager.setBaseDirectory(config.projectRoot);
        importManager.setProjectRoot(config.projectRoot); // SECURITY: enforce containment

        for (const auto& root : workspaceAdditionalRoots)
        {
            importManager.addAllowedRoot(root);
        }

        importManager.setPathAliases(buildMergedAliases(config));
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
