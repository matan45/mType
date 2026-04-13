#include "ProjectBuilder.hpp"
#include "mtclib/MtcLibBuilder.hpp"
#include "mtclib/MtcLibSerializer.hpp"
#include "mtclib/LibraryLinker.hpp"
#include "mtclib/LibrarySymbolProvider.hpp"
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
#include <sstream>
#include <cstdint>

#ifdef _WIN32
#include <process.h>    // _getpid()
#define getpid _getpid
#else
#include <unistd.h>     // getpid()
#endif

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
            reportProgress(1, 1, "Building library...");

            auto program = compileToProgram(config, environment);

            // Ensure output directory exists
            std::filesystem::path outPath(outputPath);
            if (outPath.has_parent_path() && !std::filesystem::exists(outPath.parent_path()))
            {
                std::filesystem::create_directories(outPath.parent_path());
            }

            // Build .mtcLib from compilation results
            auto exportRegistry = environment->getExportRegistry();
            auto mtcLib = mtclib::MtcLibBuilder::build(
                program, config, exportRegistry ? *exportRegistry : environment::registry::ExportRegistry{});

            // Serialize to .mtcLib file
            std::ofstream outFile(outputPath, std::ios::binary);
            if (!outFile)
            {
                throw std::runtime_error("Could not create output file: " + outputPath);
            }
            mtclib::MtcLibSerializer::serialize(mtcLib, outFile);
            outFile.close();

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

    BuildResult ProjectBuilder::buildExecutable(const ProjectConfig& config,
                                                const std::string& outputPath,
                                                const std::string& launcherPath)
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

        // Verify launcher exists
        if (!std::filesystem::exists(launcherPath))
        {
            result.errors.push_back("Launcher binary not found: " + launcherPath);
            result.success = false;
            auto endTime = std::chrono::steady_clock::now();
            result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            return result;
        }

        try
        {
            reportProgress(1, 2, "Compiling bytecode...");

            auto program = compileToProgram(config);

            // Serialize bytecode to a memory buffer
            std::ostringstream bytecodeStream;
            program.serialize(bytecodeStream);
            std::string bytecodeBlob = bytecodeStream.str();

            reportProgress(2, 2, "Embedding bytecode...");

            // Ensure output directory exists
            std::filesystem::path outPath(outputPath);
            if (outPath.has_parent_path() && !std::filesystem::exists(outPath.parent_path()))
            {
                std::filesystem::create_directories(outPath.parent_path());
            }

            // Copy launcher binary to output path
            std::filesystem::copy_file(launcherPath, outputPath,
                                       std::filesystem::copy_options::overwrite_existing);

            // Append bytecode blob + size footer + magic
            std::ofstream outFile(outputPath, std::ios::binary | std::ios::app);
            if (!outFile)
            {
                throw std::runtime_error("Could not open output file for appending: " + outputPath);
            }

            outFile.write(bytecodeBlob.data(), static_cast<std::streamsize>(bytecodeBlob.size()));

            // Write 8-byte blob size (little-endian native — see static_assert in Launcher.cpp)
            uint64_t blobSize = bytecodeBlob.size();
            outFile.write(reinterpret_cast<const char*>(&blobSize), sizeof(blobSize));

            const char magic[4] = { 'M', 'T', 'E', 'X' };
            outFile.write(magic, 4);

            outFile.close();

            result.filesCompiled = sourceFiles.size();
        }
        catch (const std::exception& e)
        {
            result.success = false;
            result.errors.push_back(e.what());

            // Remove partially written output to avoid leaving a corrupt exe (#3)
            try { std::filesystem::remove(outputPath); } catch (...) {}
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

    vm::bytecode::BytecodeProgram ProjectBuilder::compileToProgram(
        const ProjectConfig& config,
        std::shared_ptr<environment::Environment> environment)
    {
        const auto& sourceFiles = config.resolvedSourceFiles;

        // Build virtual source that imports all project files
        std::string virtualSource;
        for (const auto& sourceFile : sourceFiles)
        {
            std::filesystem::path srcPath(sourceFile);
            std::filesystem::path projRoot(config.projectRoot);
            std::filesystem::path relativePath = std::filesystem::relative(srcPath, projRoot);
            virtualSource += "import * from \"" + relativePath.string() + "\";\n";
        }

        // Write virtual source to a uniquely named temp file (PID avoids race conditions)
        std::filesystem::path tempDir = std::filesystem::temp_directory_path();
        std::string tempName = "_mtype_bundle_" + config.name + "_"
                             + std::to_string(getpid()) + ".mt";
        std::filesystem::path tempFile = tempDir / tempName;

        // RAII guard: remove temp file on scope exit even if compilation throws
        struct TempFileGuard {
            std::filesystem::path path;
            ~TempFileGuard() { std::filesystem::remove(path); }
        } tempGuard{tempFile};

        {
            std::ofstream out(tempFile);
            out << virtualSource;
        }

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
        importManager->setProjectRoot(config.projectRoot);

        for (const auto& root : workspaceAdditionalRoots)
        {
            importManager->addAllowedRoot(root);
        }

        importManager->setSearchPaths(absoluteSearchPaths);
        importManager->setPathAliases(buildMergedAliases(config));
        importManager->setCurrentFilePath(tempFile.string());

        // Allow imports from search path directories (they may be outside project root)
        for (const auto& absSearchPath : absoluteSearchPaths)
        {
            importManager->addAllowedRoot(absSearchPath);
        }

        services::ImportManager* importMgrPtr = importManager.get();

        // Create environment if not provided
        if (!environment)
        {
            environment::EnvironmentBuilder envBuilder;
            environment = envBuilder.build();
        }

        // Resolve library dependencies before compilation
        if (!config.dependencies.packages.empty())
        {
            mtclib::LibraryLinker linker(config.projectRoot);

            // Add import search paths as library search paths too
            for (const auto& sp : absoluteSearchPaths)
            {
                linker.addSearchPath(sp);
            }

            auto libraries = linker.linkDependencies(config);
            for (const auto& lib : libraries)
            {
                mtclib::LibrarySymbolProvider::registerLibrarySymbols(lib, environment);
            }
        }

        // Parse, resolve imports, optimize, compile
        lexer::Lexer lex(tempFile.string());
        parser::Parser parser(lex, std::move(importManager));
        auto ast = parser.parseProgram();

        environment->setImportManager(importMgrPtr);
        importMgrPtr->resolveAllImports(ast.get());

        services::OptimizationService optimizationService;
        ast = optimizationService.applyOptimizations(std::move(ast), environment);

        vm::compiler::BytecodeCompiler compiler(environment, true);
        return compiler.compile(ast.get());
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
