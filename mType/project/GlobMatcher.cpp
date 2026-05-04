#include "GlobMatcher.hpp"
#include <cstddef>
#include <algorithm>
#include <regex>

namespace project
{
    std::vector<std::string> GlobMatcher::matchFiles(const std::string& pattern,
                                                      const std::filesystem::path& baseDir) const
    {
        std::vector<std::string> results;

        if (!std::filesystem::exists(baseDir))
        {
            return results;
        }

        auto patternSegments = splitPath(pattern);
        collectFilesRecursive(baseDir, patternSegments, 0, results);

        std::sort(results.begin(), results.end());
        return results;
    }

    bool GlobMatcher::matches(const std::string& pattern, const std::string& path) const
    {
        auto patternSegments = splitPath(pattern);
        auto pathSegments = splitPath(path);

        return matchPatternSegments(patternSegments, 0, pathSegments, 0);
    }

    bool GlobMatcher::matchSegment(const std::string& pattern, const std::string& text) const
    {
        if (pattern == "*")
        {
            return true;
        }

        std::string regexPattern;
        regexPattern.reserve(pattern.size() * 2);

        for (char c : pattern)
        {
            switch (c)
            {
                case '*':
                    regexPattern += ".*";
                    break;
                case '?':
                    regexPattern += ".";
                    break;
                case '.':
                case '^':
                case '$':
                case '+':
                case '{':
                case '}':
                case '[':
                case ']':
                case '|':
                case '(':
                case ')':
                case '\\':
                    regexPattern += '\\';
                    regexPattern += c;
                    break;
                default:
                    regexPattern += c;
                    break;
            }
        }

        try
        {
            std::regex regex(regexPattern, std::regex::icase);
            return std::regex_match(text, regex);
        }
        catch (const std::regex_error&)
        {
            return pattern == text;
        }
    }

    std::vector<std::string> GlobMatcher::splitPath(const std::string& path) const
    {
        std::vector<std::string> segments;
        std::string current;

        for (char c : path)
        {
            if (c == '/' || c == '\\')
            {
                if (!current.empty())
                {
                    segments.push_back(current);
                    current.clear();
                }
            }
            else
            {
                current += c;
            }
        }

        if (!current.empty())
        {
            segments.push_back(current);
        }

        return segments;
    }

    bool GlobMatcher::matchPatternSegments(const std::vector<std::string>& patternSegments,
                                           size_t patternIndex,
                                           const std::vector<std::string>& pathSegments,
                                           size_t pathIndex) const
    {
        while (patternIndex < patternSegments.size() && pathIndex < pathSegments.size())
        {
            const auto& patternSeg = patternSegments[patternIndex];

            if (patternSeg == "**")
            {
                if (patternIndex + 1 >= patternSegments.size())
                {
                    return true;
                }

                for (size_t i = pathIndex; i <= pathSegments.size(); ++i)
                {
                    if (matchPatternSegments(patternSegments, patternIndex + 1, pathSegments, i))
                    {
                        return true;
                    }
                }
                return false;
            }

            if (!matchSegment(patternSeg, pathSegments[pathIndex]))
            {
                return false;
            }

            ++patternIndex;
            ++pathIndex;
        }

        while (patternIndex < patternSegments.size() && patternSegments[patternIndex] == "**")
        {
            ++patternIndex;
        }

        return patternIndex == patternSegments.size() && pathIndex == pathSegments.size();
    }

    void GlobMatcher::collectFilesRecursive(const std::filesystem::path& dir,
                                             const std::vector<std::string>& patternSegments,
                                             size_t segmentIndex,
                                             std::vector<std::string>& results) const
    {
        if (segmentIndex >= patternSegments.size())
        {
            return;
        }

        const auto& currentPattern = patternSegments[segmentIndex];
        bool isLastSegment = (segmentIndex == patternSegments.size() - 1);

        if (currentPattern == "**")
        {
            if (segmentIndex + 1 < patternSegments.size())
            {
                collectFilesRecursive(dir, patternSegments, segmentIndex + 1, results);
            }

            try
            {
                for (const auto& entry : std::filesystem::directory_iterator(dir))
                {
                    if (entry.is_directory())
                    {
                        collectFilesRecursive(entry.path(), patternSegments, segmentIndex, results);
                    }
                }
            }
            catch (const std::filesystem::filesystem_error&)
            {
            }
        }
        else
        {
            try
            {
                for (const auto& entry : std::filesystem::directory_iterator(dir))
                {
                    std::string filename = entry.path().filename().string();

                    if (matchSegment(currentPattern, filename))
                    {
                        if (isLastSegment)
                        {
                            if (entry.is_regular_file())
                            {
                                results.push_back(entry.path().string());
                            }
                        }
                        else if (entry.is_directory())
                        {
                            collectFilesRecursive(entry.path(), patternSegments, segmentIndex + 1, results);
                        }
                    }
                }
            }
            catch (const std::filesystem::filesystem_error&)
            {
            }
        }
    }
}
