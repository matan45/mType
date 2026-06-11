#include "Publish.hpp"
#include "GitSource.hpp"
#include "PackageName.hpp"
#include "PackageManifest.hpp"
#include "SemVer.hpp"
#include "Sha256.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace packagemanager
{
    namespace fs = std::filesystem;

    static void reportProgress(const PublishProgressCallback& progress, const std::string& msg)
    {
        if (progress) progress(msg);
    }

    static bool isExcludedTopLevel(const fs::path& relPath)
    {
        if (relPath.empty()) return false;
        auto it = relPath.begin();
        if (it == relPath.end()) return false;
        const std::string first = it->string();
        if (first == ".git" || first == "mt_modules") return true;
        // mtproj.lock at project root only
        if (first == "mtproj.lock" && ++it == relPath.end()) return true;
        return false;
    }

    static std::string readFileToString(const fs::path& path)
    {
        std::ifstream in(path);
        if (!in.is_open())
        {
            throw std::runtime_error("Cannot open: " + path.string());
        }
        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }

    PublishResult publish(const std::string& registryRoot,
                          const PublishOptions& opts,
                          const PublishProgressCallback& progress)
    {
        PublishResult result;
        std::string projectDir = opts.projectDir.empty() ? "." : opts.projectDir;

        try
        {
            fs::path projectPath = fs::absolute(projectDir);
            fs::path manifestPath = projectPath / "mtpkg.json";
            if (!fs::exists(manifestPath))
            {
                throw std::runtime_error("No mtpkg.json found in " + projectPath.string());
            }

            PackageManifest manifest = PackageManifest::parseFromJson(readFileToString(manifestPath));
            result.name = manifest.name;
            result.version = manifest.version;

            // Validate version is a strict semver (pre-release suffixes rejected
            // by SemVer::parse — matches the existing parser's contract).
            SemVer::parse(manifest.version);

            // Defend the path components against injection — they become
            // directory names under the registry root and could be passed to
            // git later on.
            validatePackageName(manifest.name);
            GitSource::validateSafeString(manifest.version, "version");

            fs::path registryDest = fs::path(registryRoot) / manifest.name / manifest.version;
            result.registryPath = registryDest.string();

            if (fs::exists(registryDest))
            {
                if (!opts.force)
                {
                    throw std::runtime_error(
                        "Registry entry already exists at " + registryDest.string() +
                        "; pass --force to overwrite.");
                }
                reportProgress(progress, "Overwriting existing registry entry: " + registryDest.string());
                fs::remove_all(registryDest);
            }

            fs::create_directories(registryDest);
            reportProgress(progress, "Publishing " + manifest.name + "@" + manifest.version +
                                     " to " + registryDest.string());

            // Walk the project tree and copy files, pruning excluded subtrees.
            for (auto it = fs::recursive_directory_iterator(projectPath);
                 it != fs::recursive_directory_iterator();
                 ++it)
            {
                fs::path rel = fs::relative(it->path(), projectPath);
                if (isExcludedTopLevel(rel))
                {
                    if (it->is_directory())
                    {
                        it.disable_recursion_pending();
                    }
                    continue;
                }

                fs::path dest = registryDest / rel;
                if (it->is_directory())
                {
                    fs::create_directories(dest);
                }
                else if (it->is_regular_file())
                {
                    fs::create_directories(dest.parent_path());
                    fs::copy_file(it->path(), dest, fs::copy_options::overwrite_existing);
                }
            }

            result.integrity = "sha256-" + Sha256::hashDirectory(registryDest.string());
            reportProgress(progress, "Integrity: " + result.integrity);

            if (opts.gitTag)
            {
                GitSource::createTag(manifest.version);
                reportProgress(progress,
                    "Tagged v" + manifest.version + ". Run `git push --tags` to publish.");
            }
        }
        catch (const std::exception& e)
        {
            result.success = false;
            result.errors.push_back(e.what());
        }

        return result;
    }
}
