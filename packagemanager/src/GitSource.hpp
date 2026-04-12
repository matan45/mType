#pragma once
#include "SemVer.hpp"
#include <string>
#include <vector>

namespace packagemanager
{
    class GitSource
    {
    public:
        // Parse a source string like "github:user/repo" into a clone URL.
        // Validates the URL contains only safe characters.
        static std::string toCloneUrl(const std::string& source);

        // Check if a source string is a git source
        static bool isGitSource(const std::string& source);

        // List semver tags from a remote repository (without cloning)
        static std::vector<SemVer> listRemoteTags(const std::string& cloneUrl);

        // Clone a specific tag into the local registry cache.
        // Returns the path where the package was cached.
        static std::string fetchIntoRegistry(const std::string& cloneUrl,
                                              const std::string& packageName,
                                              const std::string& version,
                                              const std::string& registryRoot);

    private:
        // Validate a URL/string contains only safe characters for git operations.
        // Rejects shell metacharacters: & | ; $ ` ( ) { } ! # etc.
        static void validateSafeString(const std::string& str, const std::string& context);

        // Run git with an argument list (no shell interpolation).
        // Returns stdout. Throws on non-zero exit.
        static std::string runGit(const std::vector<std::string>& args);

        // Run git with an argument list, return exit code (no throw).
        static int runGitStatus(const std::vector<std::string>& args);
    };
}
