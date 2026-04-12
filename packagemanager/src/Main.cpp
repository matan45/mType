#include "PackageInstaller.hpp"
#include "PackageManifest.hpp"
#include "PackageRegistry.hpp"
#include "SemVer.hpp"
#include "../../mType/project/ProjectConfigParser.hpp"
#include "../../mType/project/ProjectConfig.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

static void printUsage(const char* argv0)
{
    std::cout << "mtpm - mType Package Manager\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << argv0 << " install [project.mtproj]                          - Install all dependencies\n";
    std::cout << "  " << argv0 << " add <pkg>@<ver> [--source github:user/repo]      - Add a package\n";
    std::cout << "  " << argv0 << " remove <pkg> [.mtproj]                            - Remove a package\n";
    std::cout << "  " << argv0 << " list [.mtproj]                                    - List installed packages\n";
    std::cout << "  " << argv0 << " init <name> <version>                             - Create mtpkg.json\n";
    std::cout << "  " << argv0 << " --help                                            - Show this help\n";
    std::cout << "\nSource formats:\n";
    std::cout << "  github:user/repo                  - GitHub shorthand\n";
    std::cout << "  https://github.com/user/repo.git  - Full git URL\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << argv0 << " add mathlib@^1.0.0 --source github:matan45/mathlib\n";
    std::cout << "  " << argv0 << " install\n";
}

static std::string findMtproj(const std::string& explicitPath)
{
    if (!explicitPath.empty())
    {
        return explicitPath;
    }

    project::ProjectConfigParser parser;
    auto found = parser.findProject(".");
    if (!found)
    {
        std::cerr << "Error: No .mtproj file found in current directory or parents\n";
        std::exit(1);
    }
    return *found;
}

static std::vector<packagemanager::PackageDependency> parseDependenciesFromMtproj(
    const std::string& mtprojPath)
{
    std::vector<packagemanager::PackageDependency> deps;

    std::ifstream file(mtprojPath);
    if (!file.is_open()) return deps;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Parse <Package Name="x" Version="y" /> entries from <Dependencies> section
    size_t depsStart = content.find("<Dependencies>");
    size_t depsEnd = content.find("</Dependencies>");
    if (depsStart == std::string::npos || depsEnd == std::string::npos) return deps;

    std::string depsSection = content.substr(depsStart, depsEnd - depsStart);

    size_t pos = 0;
    while ((pos = depsSection.find("<Package", pos)) != std::string::npos)
    {
        size_t tagEnd = depsSection.find("/>", pos);
        if (tagEnd == std::string::npos) break;

        std::string tag = depsSection.substr(pos, tagEnd - pos);

        packagemanager::PackageDependency dep;

        size_t namePos = tag.find("Name=\"");
        if (namePos != std::string::npos)
        {
            namePos += 6;
            size_t nameEnd = tag.find("\"", namePos);
            if (nameEnd != std::string::npos)
            {
                dep.name = tag.substr(namePos, nameEnd - namePos);
            }
        }

        size_t verPos = tag.find("Version=\"");
        if (verPos != std::string::npos)
        {
            verPos += 9;
            size_t verEnd = tag.find("\"", verPos);
            if (verEnd != std::string::npos)
            {
                dep.versionRange = tag.substr(verPos, verEnd - verPos);
            }
        }

        size_t srcPos = tag.find("Source=\"");
        if (srcPos != std::string::npos)
        {
            srcPos += 8;
            size_t srcEnd = tag.find("\"", srcPos);
            if (srcEnd != std::string::npos)
            {
                dep.source = tag.substr(srcPos, srcEnd - srcPos);
            }
        }

        if (!dep.name.empty() && !dep.versionRange.empty())
        {
            deps.push_back(dep);
        }

        pos = tagEnd + 2;
    }

    return deps;
}

static void insertDependencyIntoMtproj(const std::string& mtprojPath,
                                        const std::string& name,
                                        const std::string& version,
                                        const std::string& source = "")
{
    std::ifstream inFile(mtprojPath);
    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string content = buffer.str();
    inFile.close();

    // Ensure <Dependencies> section exists
    if (content.find("<Dependencies>") == std::string::npos)
    {
        size_t insertPos = content.find("</Project>");
        if (insertPos == std::string::npos)
        {
            std::cerr << "Error: Invalid .mtproj format\n";
            return;
        }
        std::string depsSection = "  <Dependencies>\n  </Dependencies>\n";
        content.insert(insertPos, depsSection);
    }

    // Insert <Package> before </Dependencies>
    size_t pos = content.find("</Dependencies>");
    if (pos != std::string::npos)
    {
        std::string entry = "    <Package Name=\"" + name + "\" Version=\"" + version + "\"";
        if (!source.empty())
        {
            entry += " Source=\"" + source + "\"";
        }
        entry += " />\n  ";
        content.insert(pos, entry);
    }

    std::ofstream outFile(mtprojPath);
    outFile << content;
}

static void removeDependencyFromMtproj(const std::string& mtprojPath,
                                        const std::string& name)
{
    std::ifstream inFile(mtprojPath);
    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string content = buffer.str();
    inFile.close();

    std::string searchPattern = "Name=\"" + name + "\"";
    size_t pos = content.find(searchPattern);
    if (pos == std::string::npos)
    {
        std::cerr << "Package '" << name << "' not found in .mtproj\n";
        return;
    }

    // Find the whole <Package .../> line
    size_t lineStart = content.rfind('<', pos);
    size_t lineEnd = content.find("/>", pos);
    if (lineStart != std::string::npos && lineEnd != std::string::npos)
    {
        lineEnd += 2;
        // Remove trailing newline if present
        if (lineEnd < content.size() && content[lineEnd] == '\n') ++lineEnd;
        content.erase(lineStart, lineEnd - lineStart);
    }

    std::ofstream outFile(mtprojPath);
    outFile << content;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string command = argv[1];

    if (command == "--help" || command == "-h")
    {
        printUsage(argv[0]);
        return 0;
    }

    std::string registryRoot = packagemanager::PackageRegistry::getDefaultRegistryRoot();

    // Handle: install [.mtproj]
    if (command == "install")
    {
        std::string mtprojPath = findMtproj(argc >= 3 ? argv[2] : "");
        std::string projectRoot = std::filesystem::path(mtprojPath).parent_path().string();

        std::cout << "Loading project: " << mtprojPath << "\n";

        auto deps = parseDependenciesFromMtproj(mtprojPath);
        if (deps.empty())
        {
            std::cout << "No dependencies declared in <Dependencies> section\n";
            return 0;
        }

        std::cout << "Resolving " << deps.size() << " dependencies...\n";

        packagemanager::PackageInstaller installer(projectRoot, registryRoot);
        installer.setProgressCallback([](const std::string& msg) {
            std::cout << msg << "\n";
        });

        auto result = installer.install(deps);

        std::cout << "\nInstall " << (result.success ? "succeeded" : "failed") << "\n";
        std::cout << "  Installed: " << result.installed << "\n";
        std::cout << "  Up to date: " << result.upToDate << "\n";
        if (result.removed > 0)
        {
            std::cout << "  Removed: " << result.removed << "\n";
        }

        for (const auto& error : result.errors)
        {
            std::cerr << "  Error: " << error << "\n";
        }

        return result.success ? 0 : 1;
    }

    // Handle: add <pkg>@<version> [--source github:user/repo] [.mtproj]
    if (command == "add")
    {
        if (argc < 3)
        {
            std::cerr << "Usage: " << argv[0] << " add <package>@<version> [--source github:user/repo] [.mtproj]\n";
            return 1;
        }

        std::string pkgSpec = argv[2];
        size_t atPos = pkgSpec.find('@');
        if (atPos == std::string::npos)
        {
            std::cerr << "Error: Package specification must be name@version (e.g., mathlib@^1.0.0)\n";
            return 1;
        }

        std::string name = pkgSpec.substr(0, atPos);
        std::string version = pkgSpec.substr(atPos + 1);
        std::string source;
        std::string mtprojPath;

        // Parse remaining args for --source and .mtproj path
        for (int i = 3; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--source" && i + 1 < argc)
            {
                source = argv[++i];
            }
            else if (arg[0] != '-')
            {
                mtprojPath = arg;
            }
        }

        mtprojPath = findMtproj(mtprojPath);
        std::string projectRoot = std::filesystem::path(mtprojPath).parent_path().string();

        if (!source.empty())
        {
            std::cout << "Adding " << name << "@" << version << " from " << source << "...\n";
        }
        else
        {
            std::cout << "Adding " << name << "@" << version << "...\n";
        }

        // Update .mtproj
        insertDependencyIntoMtproj(mtprojPath, name, version, source);

        // Install
        auto deps = parseDependenciesFromMtproj(mtprojPath);
        packagemanager::PackageInstaller installer(projectRoot, registryRoot);
        installer.setProgressCallback([](const std::string& msg) {
            std::cout << msg << "\n";
        });

        auto result = installer.install(deps);

        std::cout << "\n" << (result.success ? "Added" : "Failed to add")
                  << " " << name << "@" << version << "\n";

        return result.success ? 0 : 1;
    }

    // Handle: remove <pkg> [.mtproj]
    if (command == "remove")
    {
        if (argc < 3)
        {
            std::cerr << "Usage: " << argv[0] << " remove <package> [.mtproj]\n";
            return 1;
        }

        std::string name = argv[2];
        std::string mtprojPath = findMtproj(argc >= 4 ? argv[3] : "");
        std::string projectRoot = std::filesystem::path(mtprojPath).parent_path().string();

        std::cout << "Removing " << name << "...\n";

        // Update .mtproj
        removeDependencyFromMtproj(mtprojPath, name);

        // Re-install to clean up
        auto deps = parseDependenciesFromMtproj(mtprojPath);
        packagemanager::PackageInstaller installer(projectRoot, registryRoot);
        installer.setProgressCallback([](const std::string& msg) {
            std::cout << msg << "\n";
        });

        auto result = installer.install(deps);

        std::cout << "\nRemoved " << name << "\n";
        return result.success ? 0 : 1;
    }

    // Handle: list [.mtproj]
    if (command == "list")
    {
        std::string mtprojPath = findMtproj(argc >= 3 ? argv[2] : "");
        std::string projectRoot = std::filesystem::path(mtprojPath).parent_path().string();

        packagemanager::MtModulesManager mgr(projectRoot);
        auto installed = mgr.getInstalledPackages();

        if (installed.empty())
        {
            std::cout << "No packages installed\n";
        }
        else
        {
            std::cout << "Installed packages:\n";
            for (const auto& pkg : installed)
            {
                std::cout << "  @" << pkg << "\n";
            }
        }

        return 0;
    }

    // Handle: init <name> <version>
    if (command == "init")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: " << argv[0] << " init <name> <version>\n";
            return 1;
        }

        std::string name = argv[2];
        std::string version = argv[3];

        if (std::filesystem::exists("mtpkg.json"))
        {
            std::cerr << "Error: mtpkg.json already exists\n";
            return 1;
        }

        packagemanager::PackageManifest manifest;
        manifest.name = name;
        manifest.version = version;
        manifest.sourceDir = "src";

        std::ofstream outFile("mtpkg.json");
        outFile << manifest.toJson();
        outFile.close();

        std::cout << "Created mtpkg.json for " << name << "@" << version << "\n";
        return 0;
    }

    std::cerr << "Unknown command: " << command << "\n";
    printUsage(argv[0]);
    return 1;
}
