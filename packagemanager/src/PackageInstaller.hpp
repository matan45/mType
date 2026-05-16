#pragma once
#include "DependencyResolver.hpp"
#include "PackageRegistry.hpp"
#include "Lockfile.hpp"
#include "MtModulesManager.hpp"
#include <string>
#include <functional>

namespace packagemanager
{
    struct InstallResult
    {
        bool success = true;
        size_t installed = 0;
        size_t upToDate = 0;
        size_t removed = 0;
        std::vector<std::string> errors;
    };

    using InstallProgressCallback = std::function<void(const std::string& message)>;

    class PackageInstaller
    {
    public:
        PackageInstaller(const std::string& projectRoot,
                         const std::string& registryRoot);

        void setProgressCallback(InstallProgressCallback callback);

        // When set, install() ignores any existing lockfile (bypassing
        // integrity verification) and re-resolves dependencies from scratch,
        // rewriting the lockfile with fresh hashes.
        void setForceResolve(bool value);

        // Path to mType.exe used to compile each installed package to .mtcLib.
        // When empty, the compile step is skipped (build will then fail at
        // LibraryLinker with a guiding error message).
        void setMTypeExecutable(const std::string& path);

        // Install all dependencies declared in the given list
        InstallResult install(const std::vector<PackageDependency>& dependencies);

        // Add a package: resolve, install, return updated dependency list
        InstallResult addPackage(const std::string& name,
                                 const std::string& versionRange,
                                 std::vector<PackageDependency>& dependencies);

        // Remove a package from mt_modules and return updated dependency list
        InstallResult removePackage(const std::string& name,
                                    std::vector<PackageDependency>& dependencies);

    private:
        std::string projectRoot;
        PackageRegistry registry;
        MtModulesManager modulesManager;
        InstallProgressCallback progressCallback;
        bool forceResolve = false;
        std::string mTypeExePath;

        void reportProgress(const std::string& message);

        void copyPackageToModules(const ResolvedPackage& pkg);

        std::string getLockfilePath() const;
    };
}
