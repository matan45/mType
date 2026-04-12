#include "WorkspaceConfigParser.hpp"
#include "../services/FileReader.hpp"
#include <stdexcept>
#include <unordered_set>

namespace project
{
    WorkspaceConfigParser::WorkspaceConfigParser() = default;
    WorkspaceConfigParser::~WorkspaceConfigParser() = default;

    std::unique_ptr<WorkspaceConfig> WorkspaceConfigParser::parse(const std::string& mtworkspacePath)
    {
        lexer::FileReader reader;
        std::string content = reader.readFile(mtworkspacePath);

        auto config = std::make_unique<WorkspaceConfig>();

        std::filesystem::path wsPath(mtworkspacePath);
        config->workspaceRoot = std::filesystem::canonical(wsPath.parent_path()).string();

        auto root = xmlParser.parse(content);

        if (root.tagName != "Workspace")
        {
            throw std::runtime_error("Invalid .mtworkspace file: root element must be <Workspace>");
        }

        populateConfig(root, *config);
        resolveMembers(*config);
        validateConfig(*config);

        return config;
    }

    std::optional<std::string> WorkspaceConfigParser::findWorkspace(const std::string& startDir)
    {
        std::filesystem::path current = std::filesystem::absolute(startDir);

        while (true)
        {
            for (const auto& entry : std::filesystem::directory_iterator(current))
            {
                if (entry.is_regular_file())
                {
                    std::string filename = entry.path().filename().string();
                    if (filename.size() > 12 &&
                        filename.substr(filename.size() - 12) == ".mtworkspace")
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

    void WorkspaceConfigParser::populateConfig(const XmlElement& root, WorkspaceConfig& config)
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
            if (child.tagName == "Members")
            {
                parseMembersElement(child, config);
            }
            else if (child.tagName == "Output")
            {
                parseOutputElement(child, config.output);
            }
        }
    }

    void WorkspaceConfigParser::parseMembersElement(const XmlElement& element, WorkspaceConfig& config)
    {
        for (const auto& child : element.children)
        {
            if (child.tagName == "Member")
            {
                auto pathIt = child.attributes.find("Path");
                if (pathIt == child.attributes.end())
                {
                    throw std::runtime_error("Each <Member> must have a Path attribute");
                }

                MemberProject member;
                member.path = pathIt->second;

                auto nameIt = child.attributes.find("Name");
                if (nameIt != child.attributes.end())
                {
                    member.nameOverride = nameIt->second;
                }

                config.members.push_back(std::move(member));
            }
        }
    }

    void WorkspaceConfigParser::parseOutputElement(const XmlElement& element, OutputConfig& output)
    {
        auto dirIt = element.attributes.find("Directory");
        if (dirIt != element.attributes.end())
        {
            output.directory = dirIt->second;
        }
    }

    void WorkspaceConfigParser::resolveMembers(WorkspaceConfig& config)
    {
        std::filesystem::path wsRoot(config.workspaceRoot);

        for (auto& member : config.members)
        {
            std::filesystem::path memberDir = wsRoot / member.path;

            if (!std::filesystem::exists(memberDir))
            {
                throw std::runtime_error(
                    "Member directory not found: " + member.path +
                    " (resolved to " + memberDir.string() + ")");
            }

            if (!std::filesystem::is_directory(memberDir))
            {
                throw std::runtime_error(
                    "Member path is not a directory: " + member.path);
            }

            auto mtprojPath = projectParser.findProject(memberDir.string());
            if (!mtprojPath.has_value())
            {
                throw std::runtime_error(
                    "No .mtproj file found in member directory: " + member.path);
            }

            member.config = projectParser.parse(mtprojPath.value());
        }
    }

    void WorkspaceConfigParser::validateConfig(const WorkspaceConfig& config)
    {
        if (config.name.empty())
        {
            throw std::runtime_error("Workspace name is required (use Name attribute on <Workspace>)");
        }

        if (config.members.empty())
        {
            throw std::runtime_error("Workspace must have at least one <Member>");
        }

        std::unordered_set<std::string> names;
        for (const auto& member : config.members)
        {
            std::string name = member.getName();
            if (name.empty())
            {
                throw std::runtime_error(
                    "Member at path '" + member.path + "' has no project name");
            }

            if (!names.insert(name).second)
            {
                throw std::runtime_error(
                    "Duplicate member project name: '" + name +
                    "'. Use the Name attribute on <Member> to override.");
            }
        }
    }
}
