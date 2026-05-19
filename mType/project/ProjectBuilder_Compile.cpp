#include "ProjectBuilder.hpp"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "../services/ScriptInterpreter.hpp"
#include "../services/ImportManager.hpp"
#include "../services/OptimizationService.hpp"
#include "../lexer/Lexer.hpp"
#include "../parser/Parser.hpp"
#include "../vm/compiler/BytecodeCompiler.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "mtclib/LibraryLinker.hpp"
#include "mtclib/LibrarySymbolProvider.hpp"
#include "MtModulesManager.hpp"
#include "Lockfile.hpp"

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

namespace project
{
    bool ProjectBuilder::compileFile(const std::string& sourcePath,
                                      const std::string& outputPath,
                                      const ProjectConfig& config)
    {
        try
        {
            services::ScriptInterpreter interpreter;

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
        // Precedence (lowest -> highest; later writes win via operator[]):
        //   1. workspace aliases
        //   2. mt_modules/ aliases (populated by `mtpm install`)
        //   3. project <Alias> entries
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
            // A broken mt_modules/ (permission denied, symlink loop) must not
            // kill builds that don't depend on installed packages.
        }

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

        // Build virtual source that imports all project files.
        // Use generic_string() (forward slashes) — std::filesystem::path::string()
        // returns the native form, which is backslash-separated on Windows. The
        // mType lexer interprets backslash inside a string literal as an escape
        // introducer, so src\render\Foo.mt would be tokenised as `src` + <CR> +
        // `ender\Foo.mt`, producing the ERROR_INVALID_NAME we saw in MYT-323.
        std::string virtualSource;
        for (const auto& sourceFile : sourceFiles)
        {
            std::filesystem::path srcPath(sourceFile);
            std::filesystem::path projRoot(config.projectRoot);
            std::filesystem::path relativePath = std::filesystem::relative(srcPath, projRoot);
            virtualSource += "import * from \"" + relativePath.generic_string() + "\";\n";
        }

        // Unique temp filename — PID avoids parallel-build collisions.
        std::filesystem::path tempDir = std::filesystem::temp_directory_path();
        std::string tempName = "_mtype_bundle_" + config.name + "_"
                             + std::to_string(getpid()) + ".mt";
        std::filesystem::path tempFile = tempDir / tempName;

        // RAII guard: remove temp file on scope exit even if compilation throws.
        struct TempFileGuard {
            std::filesystem::path path;
            ~TempFileGuard() { std::filesystem::remove(path); }
        } tempGuard{tempFile};

        {
            std::ofstream out(tempFile);
            out << virtualSource;
        }

        std::vector<std::string> absoluteSearchPaths;
        for (const auto& searchPath : config.imports.searchPaths)
        {
            std::filesystem::path absPath = std::filesystem::path(config.projectRoot) / searchPath;
            absoluteSearchPaths.push_back(absPath.string());
        }

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

        // Allow imports from search path directories — they may live outside project root.
        for (const auto& absSearchPath : absoluteSearchPaths)
        {
            importManager->addAllowedRoot(absSearchPath);
        }

        services::ImportManager* importMgrPtr = importManager.get();

        if (!environment)
        {
            environment::EnvironmentBuilder envBuilder;
            environment = envBuilder.build();
        }

        // Resolve library dependencies before compilation.
        //
        // mType supports two distribution models for <Package> deps:
        //   (a) Source distribution via mtpm: mtpm install populates
        //       mt_modules/@<name>/ with aliases pointing at the registry
        //       source. The user imports through `@<name>/...` and the
        //       compiler pulls source into the bundle the same way it
        //       handles local imports. No bytecode artifact is needed.
        //   (b) Pre-compiled distribution: a <name>.mtcLib file sits on a
        //       search path; LibraryLinker deserializes it and
        //       LibrarySymbolProvider registers its classes/interfaces.
        //
        // Partition the declared deps: anything with mt_modules source goes
        // through (a) and we skip it here (the alias-driven import path
        // does the work). Anything without falls through to (b). This is
        // what makes `mType.exe --build` succeed after `mtpm install`
        // without forcing a bytecode compile step — MYT-323's actual cause.
        if (!config.dependencies.packages.empty())
        {
            packagemanager::MtModulesManager modulesMgr(config.projectRoot);

            std::vector<PackageDependency> binaryOnlyDeps;
            for (const auto& dep : config.dependencies.packages)
            {
                if (!modulesMgr.isInstalled(dep.name))
                {
                    binaryOnlyDeps.push_back(dep);
                }
            }

            if (!binaryOnlyDeps.empty())
            {
                ProjectConfig linkerConfig = config;
                linkerConfig.dependencies.packages = std::move(binaryOnlyDeps);

                mtclib::LibraryLinker linker(config.projectRoot);

                for (const auto& sp : absoluteSearchPaths)
                {
                    linker.addSearchPath(sp);
                }

                // If the project has a lockfile, prefer its pinned versions over
                // declared ranges. Without this a .mtproj saying `^0.0.1` will
                // not find a versioned build at the exact installed version.
                std::filesystem::path lockPath =
                    std::filesystem::path(config.projectRoot) / "mtproj.lock";
                if (std::filesystem::exists(lockPath))
                {
                    try
                    {
                        auto lockfile = packagemanager::Lockfile::loadFromFile(lockPath.string());
                        std::unordered_map<std::string, std::string> pins;
                        for (const auto& [name, locked] : lockfile.packages)
                        {
                            pins[name] = locked.version;
                        }
                        linker.setLockfileVersions(pins);
                    }
                    catch (const std::exception& e)
                    {
                        // A corrupt lockfile must not block the build, but
                        // swallowing the error silently leaves users debugging
                        // mystery "wrong version picked" issues. Log so the
                        // failure is visible while still falling back to
                        // declared ranges. std::bad_alloc / I/O errors will
                        // also surface here — they're rare enough that the
                        // log line is the right place to learn about them.
                        std::cerr << "Warning: failed to load lockfile at '"
                                  << lockPath.string() << "': " << e.what()
                                  << ". Falling back to declared version ranges.\n";
                    }
                }

                auto libraries = linker.linkDependencies(linkerConfig);
                for (const auto& lib : libraries)
                {
                    mtclib::LibrarySymbolProvider::registerLibrarySymbols(lib, environment);
                }
            }
        }

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
}
