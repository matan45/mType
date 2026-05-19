#include "MainCommands.hpp"

#include "../ErrorReporting.hpp"
#include "../../diagnostics/TerminalDetect.hpp"
#include "../../project/DependencyGraphBuilder.hpp"
#include "../../project/DependencyGraphFormatter.hpp"
#include "../../project/ProjectConfigParser.hpp"
#include "../../project/ProjectDiscovery.hpp"
#include "../../project/WorkspaceConfigParser.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace runMain::commands
{
    std::optional<int> handleDepsCommand(int argc, char* argv[])
    {
        if (argc < 2 || std::string(argv[1]) != "--deps")
        {
            return std::nullopt;
        }

        try
        {
            // Parse flags
            std::string configPath;
            bool jsonOutput = false;
            bool dotOutput = false;
            bool showCycles = false;
            std::string whyFile;

            for (int i = 2; i < argc; ++i)
            {
                std::string arg = argv[i];
                if (arg == "--json") jsonOutput = true;
                else if (arg == "--dot") dotOutput = true;
                else if (arg == "--cycles") showCycles = true;
                else if (arg == "--why" && i + 1 < argc) whyFile = argv[++i];
                else if (arg[0] != '-') configPath = arg;
            }

            // Auto-detect project/workspace if not specified
            bool isWorkspace = false;
            if (configPath.empty())
            {
                auto discovery = project::findProjectOrWorkspace(".");
                if (!discovery)
                {
                    std::cerr << "Error: No .mtproj or .mtworkspace file found\n";
                    return 1;
                }
                configPath = discovery->path;
                isWorkspace = (discovery->type == project::DiscoveryType::WORKSPACE);
            }
            else
            {
                isWorkspace = configPath.size() > 12 &&
                             configPath.substr(configPath.size() - 12) == ".mtworkspace";
            }

            bool colorEnabled = diagnostics::TerminalDetect::isTerminal(stdout)
                             && !diagnostics::TerminalDetect::noColorRequested();
            if (colorEnabled)
            {
                diagnostics::TerminalDetect::enableVirtualTerminalProcessing(stdout);
            }

            // Build the dependency graph
            project::DependencyGraphBuilder builder;
            project::DependencyGraph graph = [&]()
            {
                if (isWorkspace)
                {
                    project::WorkspaceConfigParser wsParser;
                    auto wsConfig = wsParser.parse(configPath);
                    return builder.build(*wsConfig);
                }
                else
                {
                    project::ProjectConfigParser projParser;
                    auto projConfig = projParser.parse(configPath);
                    return builder.build(*projConfig);
                }
            }();

            // Dispatch to the appropriate output
            if (showCycles)
            {
                auto cycles = graph.findCycles();
                project::DependencyGraphFormatter::renderCycles(
                    cycles, graph.getProjectRoot(), std::cout, colorEnabled);
                return cycles.empty() ? 0 : 1;
            }

            if (!whyFile.empty())
            {
                project::DependencyGraphFormatter::renderWhy(
                    graph, whyFile, std::cout, colorEnabled);
                return 0;
            }

            if (jsonOutput)
            {
                auto json = project::DependencyGraphFormatter::toJson(graph);
                std::cout << json->toJsonString(true) << "\n";
                return 0;
            }

            if (dotOutput)
            {
                project::DependencyGraphFormatter::renderDot(graph, std::cout);
                return 0;
            }

            // Default: render tree
            project::DependencyGraphFormatter::renderTree(
                graph, std::cout, colorEnabled);
            return 0;
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }
}
