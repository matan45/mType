#pragma once
#include <functional>
#include <string>
#include <vector>

namespace packagemanager
{
    struct PublishOptions
    {
        bool force = false;
        bool gitTag = false;
        std::string projectDir;
    };

    struct PublishResult
    {
        bool success = true;
        std::string name;
        std::string version;
        std::string registryPath;
        std::string integrity;
        std::vector<std::string> errors;
    };

    using PublishProgressCallback = std::function<void(const std::string& message)>;

    // Publish the package described by {projectDir}/mtpkg.json into
    // {registryRoot}/{name}/{version}/, skipping .git, mt_modules, and
    // mtproj.lock. Computes and reports the sha256 integrity hash.
    // If opts.gitTag is set, creates a local `v<version>` git tag after copy.
    PublishResult publish(const std::string& registryRoot,
                          const PublishOptions& opts,
                          const PublishProgressCallback& progress);
}
