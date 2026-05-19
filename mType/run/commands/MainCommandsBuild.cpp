#include "MainCommands.hpp"

#include "../ErrorReporting.hpp"
#include "../../project/ProjectBuilder.hpp"
#include "../../project/ProjectConfigParser.hpp"
#include "../../project/ProjectDiscovery.hpp"
#include "../../project/WorkspaceBuilder.hpp"
#include "../../project/WorkspaceConfigParser.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace runMain::commands
{
    std::optional<int> handleBuildCommand(int argc, char* argv[])
    {
        if (argc < 2 || std::string(argv[1]) != "--build")
        {
            return std::nullopt;
        }

        std::string configPath;
        bool buildLib = false;
        bool buildExe = false;
        bool buildGui = false;

        for (int i = 2; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--lib")
            {
                buildLib = true;
            }
            else if (arg == "--exe")
            {
                buildExe = true;
            }
            else if (arg == "--gui")
            {
                buildGui = true;
            }
            else if (arg[0] != '-')
            {
                configPath = arg;
            }
        }

        if (buildGui && !buildExe)
        {
            std::cerr << "Error: --gui requires --exe.\n";
            return 1;
        }

        try
        {
            // Determine if we're building a workspace or a project
            bool isWorkspace = false;

            if (!configPath.empty())
            {
                // Explicit path: check extension
                if (configPath.size() > 12 &&
                    configPath.substr(configPath.size() - 12) == ".mtworkspace")
                {
                    isWorkspace = true;
                }
            }
            else
            {
                // Auto-detect: workspace takes priority over project
                auto discovery = project::findProjectOrWorkspace(".");
                if (!discovery)
                {
                    std::cerr << "Error: No .mtproj or .mtworkspace file found in current directory or parents\n";
                    return 1;
                }
                configPath = discovery->path;
                isWorkspace = (discovery->type == project::DiscoveryType::WORKSPACE);
            }

            if (isWorkspace)
            {
                // Workspace build
                if (buildLib)
                {
                    std::cerr << "Error: --lib is not supported for workspace builds\n";
                    return 1;
                }
                if (buildExe)
                {
                    std::cerr << "Error: --exe is not supported for workspace builds\n";
                    return 1;
                }

                project::WorkspaceConfigParser wsParser;
                std::cout << "Loading workspace: " << configPath << "\n";

                auto wsConfig = wsParser.parse(configPath);

                std::cout << "Workspace: " << wsConfig->name;
                if (!wsConfig->version.empty())
                {
                    std::cout << " v" << wsConfig->version;
                }
                std::cout << "\n";
                std::cout << "Member projects: " << wsConfig->members.size() << "\n";

                for (const auto& member : wsConfig->members)
                {
                    std::cout << "  - " << member.getName() << " (" << member.path << ")\n";
                }
                std::cout << "\n";

                project::WorkspaceBuilder builder;
                builder.setProgressCallback([](const project::WorkspaceBuildProgress& progress)
                {
                    std::cout << "[" << progress.projectName << " "
                              << progress.fileProgress.current << "/"
                              << progress.fileProgress.total << "] "
                              << progress.fileProgress.currentFile << "\n";
                });

                auto result = builder.build(*wsConfig);

                std::cout << "\nWorkspace build " << (result.success ? "succeeded" : "failed") << "\n";
                std::cout << "  Projects: " << result.projectsBuilt << " built";
                if (result.projectsFailed > 0)
                {
                    std::cout << ", " << result.projectsFailed << " failed";
                }
                std::cout << "\n";
                std::cout << "  Files:    " << result.totalFilesCompiled << " compiled";
                if (result.totalFilesFailed > 0)
                {
                    std::cout << ", " << result.totalFilesFailed << " failed";
                }
                std::cout << "\n";
                std::cout << "  Duration: " << result.duration.count() << "ms\n";

                if (!result.errors.empty())
                {
                    std::cout << "\nErrors:\n";
                    for (const auto& error : result.errors)
                    {
                        std::cout << "  " << error << "\n";
                    }
                }

                return result.success ? 0 : 1;
            }
            else
            {
                // Single project build
                project::ProjectConfigParser parser;
                std::cout << "Loading project: " << configPath << "\n";

                auto config = parser.parse(configPath);

                std::cout << "Project: " << config->name;
                if (!config->version.empty())
                {
                    std::cout << " v" << config->version;
                }
                std::cout << "\n";
                std::cout << "Source files: " << config->resolvedSourceFiles.size() << "\n";
                std::cout << "Output directory: " << config->output.directory << "\n";

                project::ProjectBuilder builder;

                builder.setProgressCallback([](const project::BuildProgress& progress)
                {
                    std::cout << "[" << progress.current << "/" << progress.total << "] " << progress.currentFile << "\n";
                });

                project::BuildResult result;

                if (buildLib)
                {
                    std::filesystem::path outputDir = std::filesystem::path(config->projectRoot) / config->output.directory;
                    std::string libPath = (outputDir / (config->name + ".mtcLib")).string();
                    std::cout << "Building library: " << libPath << "\n\n";
                    result = builder.buildLibrary(*config, libPath);
                }
                else if (buildExe)
                {
                    std::filesystem::path outputDir = std::filesystem::path(config->projectRoot) / config->output.directory;
#ifdef _WIN32
                    std::string exeName = config->name + ".exe";
#else
                    std::string exeName = config->name;
#endif
                    std::string exePath = (outputDir / exeName).string();

                    // Locate the launcher binary relative to the mType executable.
                    // --gui selects the windowed-subsystem variant (MYT-326) so the
                    // produced exe doesn't pop a console window on Explorer launch.
                    std::filesystem::path mtypePath = std::filesystem::path(argv[0]).parent_path();
#ifdef _WIN32
                    const std::string launcherName = buildGui
                        ? "mtype-launcher-gui.exe"
                        : "mtype-launcher.exe";
#else
                    const std::string launcherName = buildGui
                        ? "mtype-launcher-gui"
                        : "mtype-launcher";
#endif
                    std::string launcherPath = (mtypePath / launcherName).string();

                    std::cout << "Building executable: " << exePath << "\n\n";
                    result = builder.buildExecutable(*config, exePath, launcherPath);
                }
                else
                {
                    std::cout << "\n";
                    result = builder.build(*config);
                }

                std::cout << "\nBuild " << (result.success ? "succeeded" : "failed") << "\n";
                std::cout << "  Compiled: " << result.filesCompiled << " files\n";
                if (result.filesFailed > 0)
                {
                    std::cout << "  Failed:   " << result.filesFailed << " files\n";
                }
                std::cout << "  Duration: " << result.duration.count() << "ms\n";

                if (!result.errors.empty())
                {
                    std::cout << "\nErrors:\n";
                    for (const auto& error : result.errors)
                    {
                        std::cout << "  " << error << "\n";
                    }
                }

                return result.success ? 0 : 1;
            }
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }
}
