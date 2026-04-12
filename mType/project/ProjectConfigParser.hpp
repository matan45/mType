#pragma once
#include "ProjectConfig.hpp"
#include "GlobMatcher.hpp"
#include "XmlParser.hpp"
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
        XmlParser xmlParser;

        void populateConfig(const XmlElement& root, ProjectConfig& config);

        void parseSourceElement(const XmlElement& element, SourceConfig& source);

        void parseOutputElement(const XmlElement& element, OutputConfig& output);

        void parseImportsElement(const XmlElement& element, ImportsConfig& imports);

        void parseDependenciesElement(const XmlElement& element, DependenciesConfig& deps);

        void resolveSourceFiles(ProjectConfig& config);

        void validateConfig(const ProjectConfig& config);
    };
}
