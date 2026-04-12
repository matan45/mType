#include "ProjectConfigParser.hpp"
#include "../services/FileReader.hpp"
#include <stdexcept>
#include <algorithm>
#include <set>

namespace project
{
    ProjectConfigParser::ProjectConfigParser() = default;
    ProjectConfigParser::~ProjectConfigParser() = default;

    std::unique_ptr<ProjectConfig> ProjectConfigParser::parse(const std::string& mtprojPath)
    {
        lexer::FileReader reader;
        std::string content = reader.readFile(mtprojPath);

        auto config = std::make_unique<ProjectConfig>();

        std::filesystem::path projPath(mtprojPath);
        config->projectRoot = std::filesystem::canonical(projPath.parent_path()).string();

        auto root = xmlParser.parse(content);

        if (root.tagName != "Project")
        {
            throw std::runtime_error("Invalid .mtproj file: root element must be <Project>");
        }

        populateConfig(root, *config);
        resolveSourceFiles(*config);
        validateConfig(*config);

        return config;
    }

    std::optional<std::string> ProjectConfigParser::findProject(const std::string& startDir)
    {
        std::filesystem::path current = std::filesystem::absolute(startDir);

        while (true)
        {
            for (const auto& entry : std::filesystem::directory_iterator(current))
            {
                if (entry.is_regular_file())
                {
                    std::string filename = entry.path().filename().string();
                    if (filename.size() > 7 && filename.substr(filename.size() - 7) == ".mtproj")
                    {
                        return entry.path().string();
                    }
                }
            }

            auto parent = current.parent_path();
            if (parent == current)
            {
                break;
            }
            current = parent;
        }

        return std::nullopt;
    }

    void ProjectConfigParser::populateConfig(const XmlElement& root, ProjectConfig& config)
    {
        auto nameIt = root.attributes.find("Name");
        if (nameIt != root.attributes.end())
        {
            config.name = nameIt->second;
        }

        auto versionIt = root.attributes.find("Version");
        if (versionIt != root.attributes.end())
        {
            config.version = versionIt->second;
        }

        for (const auto& child : root.children)
        {
            if (child.tagName == "Source")
            {
                parseSourceElement(child, config.source);
            }
            else if (child.tagName == "Output")
            {
                parseOutputElement(child, config.output);
            }
            else if (child.tagName == "Imports")
            {
                parseImportsElement(child, config.imports);
            }
            else if (child.tagName == "Dependencies")
            {
                parseDependenciesElement(child, config.dependencies);
            }
        }
    }

    void ProjectConfigParser::parseSourceElement(const XmlElement& element, SourceConfig& source)
    {
        for (const auto& child : element.children)
        {
            if (child.tagName == "Include")
            {
                if (!child.content.empty())
                {
                    source.include.push_back(child.content);
                }
            }
            else if (child.tagName == "Exclude")
            {
                if (!child.content.empty())
                {
                    source.exclude.push_back(child.content);
                }
            }
        }
    }

    void ProjectConfigParser::parseOutputElement(const XmlElement& element, OutputConfig& output)
    {
        auto dirIt = element.attributes.find("Directory");
        if (dirIt != element.attributes.end())
        {
            output.directory = dirIt->second;
        }
    }

    void ProjectConfigParser::parseImportsElement(const XmlElement& element, ImportsConfig& imports)
    {
        for (const auto& child : element.children)
        {
            if (child.tagName == "SearchPath")
            {
                if (!child.content.empty())
                {
                    imports.searchPaths.push_back(child.content);
                }
            }
            else if (child.tagName == "Alias")
            {
                auto nameIt = child.attributes.find("Name");
                auto pathIt = child.attributes.find("Path");

                if (nameIt != child.attributes.end() && pathIt != child.attributes.end())
                {
                    imports.aliases[nameIt->second] = pathIt->second;
                }
            }
        }
    }

    void ProjectConfigParser::parseDependenciesElement(const XmlElement& element, DependenciesConfig& deps)
    {
        for (const auto& child : element.children)
        {
            if (child.tagName == "Package")
            {
                auto nameIt = child.attributes.find("Name");
                auto versionIt = child.attributes.find("Version");

                if (nameIt != child.attributes.end() && versionIt != child.attributes.end())
                {
                    PackageDependency dep;
                    dep.name = nameIt->second;
                    dep.versionRange = versionIt->second;

                    auto sourceIt = child.attributes.find("Source");
                    if (sourceIt != child.attributes.end())
                    {
                        dep.source = sourceIt->second;
                    }

                    deps.packages.push_back(dep);
                }
            }
        }
    }

    void ProjectConfigParser::resolveSourceFiles(ProjectConfig& config)
    {
        std::set<std::string> includedFiles;
        std::set<std::string> excludedFiles;

        std::filesystem::path baseDir(config.projectRoot);

        for (const auto& pattern : config.source.include)
        {
            auto files = globMatcher.matchFiles(pattern, baseDir);
            for (const auto& file : files)
            {
                includedFiles.insert(file);
            }
        }

        for (const auto& pattern : config.source.exclude)
        {
            auto files = globMatcher.matchFiles(pattern, baseDir);
            for (const auto& file : files)
            {
                excludedFiles.insert(file);
            }
        }

        for (const auto& file : includedFiles)
        {
            if (excludedFiles.find(file) == excludedFiles.end())
            {
                config.resolvedSourceFiles.push_back(file);
            }
        }

        std::sort(config.resolvedSourceFiles.begin(), config.resolvedSourceFiles.end());
    }

    void ProjectConfigParser::validateConfig(const ProjectConfig& config)
    {
        if (config.name.empty())
        {
            throw std::runtime_error("Project name is required (use Name attribute on <Project>)");
        }

        if (config.source.include.empty())
        {
            throw std::runtime_error("At least one <Include> pattern is required in <Source>");
        }

        if (config.resolvedSourceFiles.empty())
        {
            throw std::runtime_error("No source files matched the include patterns");
        }
    }
}
