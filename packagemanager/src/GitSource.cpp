#include "GitSource.hpp"
#include <filesystem>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>

namespace packagemanager
{
    namespace fs = std::filesystem;

    static bool startsWith(const std::string& str, const std::string& prefix)
    {
        return str.size() >= prefix.size() &&
               str.compare(0, prefix.size(), prefix) == 0;
    }

    std::string GitSource::toCloneUrl(const std::string& source)
    {
        if (startsWith(source, "github:"))
        {
            return "https://github.com/" + source.substr(7) + ".git";
        }

        if (startsWith(source, "https://") || startsWith(source, "http://") ||
            startsWith(source, "git://"))
        {
            return source;
        }

        throw std::runtime_error("Unknown git source format: " + source);
    }

    bool GitSource::isGitSource(const std::string& source)
    {
        return startsWith(source, "github:") ||
               startsWith(source, "https://") ||
               startsWith(source, "http://") ||
               startsWith(source, "git://");
    }

    std::string GitSource::runCommand(const std::string& command)
    {
        std::string result;

#ifdef _WIN32
        FILE* pipe = _popen(command.c_str(), "r");
#else
        FILE* pipe = popen(command.c_str(), "r");
#endif

        if (!pipe)
        {
            throw std::runtime_error("Failed to run command: " + command);
        }

        std::array<char, 256> buffer;
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
        {
            result += buffer.data();
        }

#ifdef _WIN32
        int status = _pclose(pipe);
#else
        int status = pclose(pipe);
#endif

        if (status != 0)
        {
            throw std::runtime_error("Command failed (exit " + std::to_string(status) + "): " + command);
        }

        return result;
    }

    int GitSource::runCommandStatus(const std::string& command)
    {
        return std::system(command.c_str());
    }

    // Parse a semver from a tag ref like "refs/tags/v1.2.3" or "refs/tags/1.2.3"
    // Returns empty string if not a valid semver tag
    static std::string extractVersionFromTagRef(const std::string& line)
    {
        const std::string prefix = "refs/tags/";
        size_t pos = line.find(prefix);
        if (pos == std::string::npos)
        {
            return "";
        }

        std::string tag = line.substr(pos + prefix.size());

        // Trim whitespace
        while (!tag.empty() && (tag.back() == '\n' || tag.back() == '\r' || tag.back() == ' '))
        {
            tag.pop_back();
        }

        // Skip dereferenced tags (^{})
        if (tag.find("^{") != std::string::npos)
        {
            return "";
        }

        // Strip optional 'v' prefix
        std::string version = tag;
        if (!version.empty() && version[0] == 'v')
        {
            version = version.substr(1);
        }

        // Validate it looks like X.Y.Z
        int dots = 0;
        for (char c : version)
        {
            if (c == '.') ++dots;
            else if (c < '0' || c > '9') return "";
        }
        if (dots != 2) return "";

        return version;
    }

    std::vector<SemVer> GitSource::listRemoteTags(const std::string& cloneUrl)
    {
        std::string output = runCommand("git ls-remote --tags " + cloneUrl + " 2>&1");

        std::vector<SemVer> versions;
        std::istringstream stream(output);
        std::string line;

        while (std::getline(stream, line))
        {
            std::string version = extractVersionFromTagRef(line);
            if (!version.empty())
            {
                try
                {
                    versions.push_back(SemVer::parse(version));
                }
                catch (const std::exception&)
                {
                    // Skip unparseable tags
                }
            }
        }

        std::sort(versions.begin(), versions.end());
        return versions;
    }

    std::string GitSource::fetchIntoRegistry(const std::string& cloneUrl,
                                              const std::string& packageName,
                                              const std::string& version,
                                              const std::string& registryRoot)
    {
        fs::path destPath = fs::path(registryRoot) / packageName / version;

        // If already cached, return immediately
        if (fs::exists(destPath / "mtpkg.json"))
        {
            return destPath.string();
        }

        // Create registry directory
        fs::create_directories(destPath);

        // Clone into a temp directory, then copy
        fs::path tempDir = fs::temp_directory_path() / ("_mtype_git_" + packageName + "_" + version);
        if (fs::exists(tempDir))
        {
            fs::remove_all(tempDir);
        }

        // Try tag with 'v' prefix first, then without
        std::string tag = "v" + version;
        std::string cloneCmd = "git clone --depth 1 --branch " + tag +
                               " " + cloneUrl + " \"" + tempDir.string() + "\" 2>&1";

        int result = runCommandStatus(cloneCmd);
        if (result != 0)
        {
            tag = version;
            cloneCmd = "git clone --depth 1 --branch " + tag +
                       " " + cloneUrl + " \"" + tempDir.string() + "\" 2>&1";
            result = runCommandStatus(cloneCmd);
        }

        if (result != 0)
        {
            if (fs::exists(tempDir)) fs::remove_all(tempDir);
            if (fs::exists(destPath)) fs::remove_all(destPath);
            throw std::runtime_error(
                "Failed to clone " + cloneUrl + " at tag " + version +
                ". Ensure the repo exists and has a matching tag (v" + version + " or " + version + ").");
        }

        // Remove .git directory (we don't need history in the registry)
        fs::path gitDir = tempDir / ".git";
        if (fs::exists(gitDir))
        {
            fs::remove_all(gitDir);
        }

        // Copy to registry
        fs::copy(tempDir, destPath, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

        // Clean up temp
        fs::remove_all(tempDir);

        // Verify mtpkg.json exists
        if (!fs::exists(destPath / "mtpkg.json"))
        {
            throw std::runtime_error(
                "Cloned " + cloneUrl + "@" + version +
                " but no mtpkg.json found. Is this an mType package?");
        }

        return destPath.string();
    }
}
