#pragma once
#include "WorkspaceConfig.hpp"
#include "ProjectConfigParser.hpp"
#include "XmlParser.hpp"
#include <memory>
#include <optional>
#include <filesystem>

namespace project
{
    class WorkspaceConfigParser
    {
    public:
        WorkspaceConfigParser();
        ~WorkspaceConfigParser();

        std::unique_ptr<WorkspaceConfig> parse(const std::string& mtworkspacePath);

        std::optional<std::string> findWorkspace(const std::string& startDir = ".");

    private:
        XmlParser xmlParser;
        ProjectConfigParser projectParser;

        void populateConfig(const XmlElement& root, WorkspaceConfig& config);

        void parseMembersElement(const XmlElement& element, WorkspaceConfig& config);

        void parseOutputElement(const XmlElement& element, OutputConfig& output);

        void resolveMembers(WorkspaceConfig& config);

        void validateConfig(const WorkspaceConfig& config);
    };
}
