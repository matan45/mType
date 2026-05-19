#include "MainCommands.hpp"

#include "../ErrorReporting.hpp"
#include "../../project/ProjectBuilder.hpp"
#include "../../project/ProjectConfigParser.hpp"
#include "../../project/ProjectDiscovery.hpp"
#include "../../project/WorkspaceBuilder.hpp"
#include "../../project/WorkspaceConfigParser.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace runMain::commands
{
    std::optional<int> handleCleanCommand(int argc, char* argv[])
    {
        if (argc < 2 || std::string(argv[1]) != "--clean")
        {
            return std::nullopt;
        }

        std::string configPath;

        for (int i = 2; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg[0] != '-')
            {
                configPath = arg;
                break;
            }
        }

        try
        {
            bool isWorkspace = false;

            if (!configPath.empty())
            {
                if (configPath.size() > 12 &&
                    configPath.substr(configPath.size() - 12) == ".mtworkspace")
                {
                    isWorkspace = true;
                }
            }
            else
            {
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
                project::WorkspaceConfigParser wsParser;
                auto wsConfig = wsParser.parse(configPath);

                project::WorkspaceBuilder builder;
                builder.clean(*wsConfig);

                std::cout << "Clean completed for workspace: " << wsConfig->name << "\n";
                for (const auto& member : wsConfig->members)
                {
                    if (member.config)
                    {
                        std::cout << "  - " << member.getName() << ": " << member.config->output.directory << "\n";
                    }
                }
            }
            else
            {
                project::ProjectConfigParser parser;
                auto config = parser.parse(configPath);

                project::ProjectBuilder builder;
                builder.clean(*config);

                std::cout << "Clean completed. Removed: " << config->output.directory << "\n";
            }

            return 0;
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }
}
