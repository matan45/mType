#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace project
{
    class GlobMatcher
    {
    public:
        GlobMatcher() = default;
        ~GlobMatcher() = default;

        std::vector<std::string> matchFiles(const std::string& pattern,
                                            const std::filesystem::path& baseDir) const;

        bool matches(const std::string& pattern, const std::string& path) const;

    private:
        bool matchSegment(const std::string& pattern, const std::string& text) const;

        std::vector<std::string> splitPath(const std::string& path) const;

        bool matchPatternSegments(const std::vector<std::string>& patternSegments,
                                  size_t patternIndex,
                                  const std::vector<std::string>& pathSegments,
                                  size_t pathIndex) const;

        void collectFilesRecursive(const std::filesystem::path& dir,
                                   const std::vector<std::string>& patternSegments,
                                   size_t segmentIndex,
                                   std::vector<std::string>& results) const;
    };
}
