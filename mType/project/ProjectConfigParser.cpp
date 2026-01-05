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

        auto root = parseXml(content);

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

    void ProjectConfigParser::skipWhitespace(const std::string& xml, size_t& pos)
    {
        while (pos < xml.size() && std::isspace(static_cast<unsigned char>(xml[pos])))
        {
            ++pos;
        }
    }

    std::string ProjectConfigParser::parseTagName(const std::string& xml, size_t& pos)
    {
        std::string name;
        while (pos < xml.size() && !std::isspace(static_cast<unsigned char>(xml[pos])) &&
               xml[pos] != '>' && xml[pos] != '/')
        {
            name += xml[pos++];
        }
        return name;
    }

    std::unordered_map<std::string, std::string> ProjectConfigParser::parseAttributes(const std::string& xml,
                                                                                        size_t& pos)
    {
        std::unordered_map<std::string, std::string> attrs;

        while (pos < xml.size())
        {
            skipWhitespace(xml, pos);

            if (pos >= xml.size() || xml[pos] == '>' || xml[pos] == '/')
            {
                break;
            }

            std::string attrName;
            while (pos < xml.size() && xml[pos] != '=' && !std::isspace(static_cast<unsigned char>(xml[pos])))
            {
                attrName += xml[pos++];
            }

            skipWhitespace(xml, pos);

            if (pos >= xml.size() || xml[pos] != '=')
            {
                continue;
            }
            ++pos;

            skipWhitespace(xml, pos);

            if (pos >= xml.size() || xml[pos] != '"')
            {
                continue;
            }
            ++pos;

            std::string attrValue;
            while (pos < xml.size() && xml[pos] != '"')
            {
                attrValue += xml[pos++];
            }

            if (pos < xml.size())
            {
                ++pos;
            }

            attrs[attrName] = attrValue;
        }

        return attrs;
    }

    ProjectConfigParser::XmlElement ProjectConfigParser::parseXml(const std::string& xml)
    {
        size_t pos = 0;
        skipWhitespace(xml, pos);

        if (xml.substr(pos, 5) == "<?xml")
        {
            pos = xml.find("?>", pos);
            if (pos != std::string::npos)
            {
                pos += 2;
            }
            skipWhitespace(xml, pos);
        }

        if (pos >= xml.size() || xml[pos] != '<')
        {
            throw std::runtime_error("Invalid XML: expected '<'");
        }
        ++pos;

        XmlElement element;
        element.tagName = parseTagName(xml, pos);
        element.attributes = parseAttributes(xml, pos);

        skipWhitespace(xml, pos);

        if (pos < xml.size() && xml[pos] == '/')
        {
            ++pos;
            if (pos < xml.size() && xml[pos] == '>')
            {
                ++pos;
            }
            return element;
        }

        if (pos < xml.size() && xml[pos] == '>')
        {
            ++pos;
        }

        element.children = parseChildren(xml, pos, element.tagName);

        return element;
    }

    std::vector<ProjectConfigParser::XmlElement> ProjectConfigParser::parseChildren(const std::string& xml,
                                                                                     size_t& pos,
                                                                                     const std::string& parentTag)
    {
        std::vector<XmlElement> children;
        std::string textContent;

        while (pos < xml.size())
        {
            if (xml[pos] == '<')
            {
                if (pos + 1 < xml.size() && xml[pos + 1] == '/')
                {
                    pos += 2;
                    std::string closingTag = parseTagName(xml, pos);

                    while (pos < xml.size() && xml[pos] != '>')
                    {
                        ++pos;
                    }
                    if (pos < xml.size())
                    {
                        ++pos;
                    }

                    break;
                }

                ++pos;
                XmlElement child;
                child.tagName = parseTagName(xml, pos);
                child.attributes = parseAttributes(xml, pos);

                skipWhitespace(xml, pos);

                if (pos < xml.size() && xml[pos] == '/')
                {
                    ++pos;
                    if (pos < xml.size() && xml[pos] == '>')
                    {
                        ++pos;
                    }
                    children.push_back(child);
                }
                else if (pos < xml.size() && xml[pos] == '>')
                {
                    ++pos;

                    size_t contentStart = pos;
                    size_t depth = 1;
                    size_t childContentEnd = pos;

                    while (pos < xml.size() && depth > 0)
                    {
                        if (xml[pos] == '<')
                        {
                            if (pos + 1 < xml.size() && xml[pos + 1] == '/')
                            {
                                --depth;
                                if (depth == 0)
                                {
                                    childContentEnd = pos;
                                }
                            }
                            else if (pos + 1 < xml.size() && xml[pos + 1] != '!')
                            {
                                ++depth;
                            }
                        }
                        ++pos;
                    }

                    std::string innerContent = xml.substr(contentStart, childContentEnd - contentStart);

                    size_t innerPos = 0;
                    bool hasChildElements = false;
                    while (innerPos < innerContent.size())
                    {
                        if (innerContent[innerPos] == '<' && innerPos + 1 < innerContent.size() &&
                            innerContent[innerPos + 1] != '/')
                        {
                            hasChildElements = true;
                            break;
                        }
                        ++innerPos;
                    }

                    if (hasChildElements)
                    {
                        size_t parsePos = contentStart;
                        child.children = parseChildren(xml, parsePos, child.tagName);
                    }
                    else
                    {
                        size_t start = 0;
                        size_t end = innerContent.size();
                        while (start < end && std::isspace(static_cast<unsigned char>(innerContent[start])))
                        {
                            ++start;
                        }
                        while (end > start && std::isspace(static_cast<unsigned char>(innerContent[end - 1])))
                        {
                            --end;
                        }
                        child.content = innerContent.substr(start, end - start);
                    }

                    while (pos < xml.size() && xml[pos] != '>')
                    {
                        ++pos;
                    }
                    if (pos < xml.size())
                    {
                        ++pos;
                    }

                    children.push_back(child);
                }
            }
            else
            {
                ++pos;
            }
        }

        return children;
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
