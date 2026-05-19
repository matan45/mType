#include "MainCommands.hpp"

#include "../ErrorReporting.hpp"
#include "../../project/ProjectConfigParser.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace runMain::commands
{
    std::optional<int> handleInitCommand(int argc, char* argv[])
    {
        if (argc < 4 || std::string(argv[1]) != "--init")
        {
            return std::nullopt;
        }

        std::string projectName = argv[2];
        std::string includePath = argv[3];

        std::string filename = projectName + ".mtproj";

        if (std::filesystem::exists(filename))
        {
            std::cerr << "Error: " << filename << " already exists\n";
            return 1;
        }

        std::ofstream outFile(filename);
        if (!outFile)
        {
            std::cerr << "Error: Could not create " << filename << "\n";
            return 1;
        }

        outFile << "<Project Name=\"" << projectName << "\" Version=\"1.0.0\">\n";
        outFile << "  <Source>\n";
        outFile << "    <Include>" << includePath << "</Include>\n";
        outFile << "  </Source>\n";
        outFile << "  <Output Directory=\"build\" />\n";
        outFile << "  <Imports>\n";
        outFile << "  </Imports>\n";
        outFile << "</Project>\n";

        outFile.close();
        std::cout << "Created " << filename << "\n";

        // Scaffold a main.mt entry-point file derived from the include pattern.
        // Wildcard pattern -> directory prefix + main.mt; non-glob -> verbatim.
        std::filesystem::path mainPath;
        auto firstWild = includePath.find_first_of("*?");
        if (firstWild == std::string::npos)
        {
            mainPath = includePath;
        }
        else
        {
            std::string before = includePath.substr(0, firstWild);
            while (!before.empty() && (before.back() == '/' || before.back() == '\\'))
            {
                before.pop_back();
            }
            mainPath = before.empty()
                ? std::filesystem::path("main.mt")
                : std::filesystem::path(before) / "main.mt";
        }

        if (mainPath.extension() == ".mt" && !std::filesystem::exists(mainPath))
        {
            auto parent = mainPath.parent_path();
            if (!parent.empty())
            {
                std::filesystem::create_directories(parent);
            }
            std::ofstream mainFile(mainPath);
            if (mainFile)
            {
                mainFile << "@EntryPoint\n";
                mainFile << "class App {\n";
                mainFile << "    public static function main(string[] args): void {\n";
                mainFile << "    }\n";
                mainFile << "}\n";
                mainFile.close();
                std::cout << "Created " << mainPath.string() << "\n";
            }
        }

        return 0;
    }

    std::optional<int> handleInitWorkspaceCommand(int argc, char* argv[])
    {
        if (argc < 3 || std::string(argv[1]) != "--init-workspace")
        {
            return std::nullopt;
        }

        std::string workspaceName = argv[2];
        std::string filename = workspaceName + ".mtworkspace";

        if (std::filesystem::exists(filename))
        {
            std::cerr << "Error: " << filename << " already exists\n";
            return 1;
        }

        std::ofstream outFile(filename);
        if (!outFile)
        {
            std::cerr << "Error: Could not create " << filename << "\n";
            return 1;
        }

        outFile << "<Workspace Name=\"" << workspaceName << "\" Version=\"1.0.0\">\n";
        outFile << "  <Members>\n";

        // Add any member paths provided as extra arguments
        for (int i = 3; i < argc; ++i)
        {
            outFile << "    <Member Path=\"" << argv[i] << "\" />\n";
        }

        outFile << "  </Members>\n";
        outFile << "  <Output Directory=\"build\" />\n";
        outFile << "</Workspace>\n";

        outFile.close();

        std::cout << "Created " << filename << "\n";
        return 0;
    }

    std::optional<int> handleAddCommand(int argc, char* argv[])
    {
        if (argc < 3 || std::string(argv[1]) != "--add")
        {
            return std::nullopt;
        }

        std::string pattern = argv[2];
        std::string mtprojPath;

        if (argc >= 4)
        {
            mtprojPath = argv[3];
        }
        else
        {
            project::ProjectConfigParser parser;
            auto found = parser.findProject(".");
            if (!found)
            {
                std::cerr << "Error: No .mtproj file found\n";
                return 1;
            }
            mtprojPath = *found;
        }

        try
        {
            std::ifstream inFile(mtprojPath);
            if (!inFile)
            {
                std::cerr << "Error: Could not open " << mtprojPath << "\n";
                return 1;
            }

            std::stringstream buffer;
            buffer << inFile.rdbuf();
            std::string content = buffer.str();
            inFile.close();

            // Find </Source> and insert before it
            std::string searchTag = "</Source>";
            size_t pos = content.find(searchTag);
            if (pos == std::string::npos)
            {
                std::cerr << "Error: Invalid .mtproj format (missing </Source>)\n";
                return 1;
            }

            std::string newInclude = "    <Include>" + pattern + "</Include>\n  ";
            content.insert(pos, newInclude);

            std::ofstream outFile(mtprojPath);
            outFile << content;
            outFile.close();

            std::cout << "Added include pattern: " << pattern << "\n";
            return 0;
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }

    std::optional<int> handleRemoveCommand(int argc, char* argv[])
    {
        if (argc < 3 || std::string(argv[1]) != "--remove")
        {
            return std::nullopt;
        }

        std::string pattern = argv[2];
        std::string mtprojPath;

        if (argc >= 4)
        {
            mtprojPath = argv[3];
        }
        else
        {
            project::ProjectConfigParser parser;
            auto found = parser.findProject(".");
            if (!found)
            {
                std::cerr << "Error: No .mtproj file found\n";
                return 1;
            }
            mtprojPath = *found;
        }

        try
        {
            std::ifstream inFile(mtprojPath);
            if (!inFile)
            {
                std::cerr << "Error: Could not open " << mtprojPath << "\n";
                return 1;
            }

            std::stringstream buffer;
            buffer << inFile.rdbuf();
            std::string content = buffer.str();
            inFile.close();

            // Find and remove the include line
            std::string searchPattern = "<Include>" + pattern + "</Include>";
            size_t pos = content.find(searchPattern);
            if (pos == std::string::npos)
            {
                std::cerr << "Error: Pattern not found in project: " << pattern << "\n";
                return 1;
            }

            // Find the start of the line (go back to newline or start)
            size_t lineStart = pos;
            while (lineStart > 0 && content[lineStart - 1] != '\n')
            {
                --lineStart;
            }

            // Find end of line
            size_t lineEnd = pos + searchPattern.length();
            while (lineEnd < content.length() && content[lineEnd] != '\n')
            {
                ++lineEnd;
            }
            if (lineEnd < content.length())
            {
                ++lineEnd; // Include the newline
            }

            content.erase(lineStart, lineEnd - lineStart);

            std::ofstream outFile(mtprojPath);
            outFile << content;
            outFile.close();

            std::cout << "Removed include pattern: " << pattern << "\n";
            return 0;
        }
        catch (const std::exception& e)
        {
            runMain::reportException(e);
            return 1;
        }
    }
}
