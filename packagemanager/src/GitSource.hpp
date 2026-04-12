#pragma once
#include "SemVer.hpp"
#include <string>
#include <vector>

namespace packagemanager
{
    class GitSource
    {
    public:
        // Parse a source string like "github:user/repo" into a clone URL
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
        // Run a shell command and capture stdout
        static std::string runCommand(const std::string& command);

        // Run a shell command, return exit code
        static int runCommandStatus(const std::string& command);
    };
}
