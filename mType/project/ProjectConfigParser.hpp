#pragma once
#include "ProjectConfig.hpp"
#include "GlobMatcher.hpp"
#include <memory>
#include <optional>
#include <filesystem>

namespace project
{
    class ProjectConfigParser
    {
    public:
        ProjectConfigParser();
        ~ProjectConfigParser();

        std::unique_ptr<ProjectConfig> parse(const std::string& mtprojPath);

        std::optional<std::string> findProject(const std::string& startDir = ".");

    private:
        GlobMatcher globMatcher;

        struct XmlElement
        {
            std::string tagName;
            std::unordered_map<std::string, std::string> attributes;
            std::string content;
            std::vector<XmlElement> children;
        };

        XmlElement parseXml(const std::string& xml);

        void skipWhitespace(const std::string& xml, size_t& pos);

        std::string parseTagName(const std::string& xml, size_t& pos);

        std::unordered_map<std::string, std::string> parseAttributes(const std::string& xml, size_t& pos);

        std::string parseContent(const std::string& xml, size_t& pos, const std::string& tagName);

        std::vector<XmlElement> parseChildren(const std::string& xml, size_t& pos, const std::string& parentTag);

        void populateConfig(const XmlElement& root, ProjectConfig& config);

        void parseSourceElement(const XmlElement& element, SourceConfig& source);

        void parseOutputElement(const XmlElement& element, OutputConfig& output);

        void parseImportsElement(const XmlElement& element, ImportsConfig& imports);

        void resolveSourceFiles(ProjectConfig& config);

        void validateConfig(const ProjectConfig& config);
    };
}
