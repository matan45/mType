#include "PackageInstaller.hpp"
#include "PackageCompiler.hpp"
#include "Sha256.hpp"
#include <filesystem>
#include <iostream>

namespace packagemanager
{
    namespace fs = std::filesystem;

    PackageInstaller::PackageInstaller(const std::string& projectRoot,
                                       const std::string& registryRoot)
        : projectRoot(projectRoot)
        , registry(registryRoot)
        , modulesManager(projectRoot)
    {
    }

    void PackageInstaller::setProgressCallback(InstallProgressCallback callback)
    {
        progressCallback = std::move(callback);
    }

    void PackageInstaller::setForceResolve(bool value)
    {
        forceResolve = value;
    }

    void PackageInstaller::setMTypeExecutable(const std::string& path)
    {
        mTypeExePath = path;
    }

    void PackageInstaller::reportProgress(const std::string& message)
    {
        if (progressCallback)
        {
            progressCallback(message);
        }
    }

    std::string PackageInstaller::getLockfilePath() const
    {
        return (fs::path(projectRoot) / "mtproj.lock").string();
    }

    InstallResult PackageInstaller::install(const std::vector<PackageDependency>& dependencies)
    {
        InstallResult result;

        if (dependencies.empty())
        {
            reportProgress("No dependencies to install");
            return result;
        }

        try
        {
            // Check if lockfile exists and is fresh
            std::string lockfilePath = getLockfilePath();
            bool useLockfile = false;
            Lockfile lockfile;

            if (fs::exists(lockfilePath) && !forceResolve)
            {
                try
                {
                    lockfile = Lockfile::loadFromFile(lockfilePath);
                    // Verify all declared deps are in lockfile
                    useLockfile = true;
                    for (const auto& dep : dependencies)
                    {
                        if (lockfile.packages.find(dep.name) == lockfile.packages.end())
                        {
                            useLockfile = false;
                            break;
                        }
                    }
                }
                catch (const std::exception&)
                {
                    useLockfile = false;
                }
            }
            else if (forceResolve && fs::exists(lockfilePath))
            {
                reportProgress("Ignoring lockfile (--force-resolve)");
            }

            // Resolve dependencies
            std::unordered_map<std::string, ResolvedPackage> resolved;

            // Register git sources so ensureCached can fetch if needed
            for (const auto& dep : dependencies)
            {
                if (!dep.source.empty())
                {
                    registry.registerGitSource(dep.name, dep.source);
                }
            }

            if (useLockfile)
            {
                reportProgress("Using lockfile for version resolution");
                for (const auto& [name, locked] : lockfile.packages)
                {
                    // Ensure locked version is cached (fetches from git if needed)
                    registry.ensureCached(name, locked.version);

                    ResolvedPackage pkg;
                    pkg.name = name;
                    pkg.version = locked.version;
                    pkg.registryPath = registry.getPackagePath(name, locked.version);
                    pkg.dependencies = locked.dependencies;
                    resolved[name] = pkg;
                }

                // Verify the on-disk registry contents still match the
                // integrity hashes recorded in the lockfile.
                for (const auto& [name, pkg] : resolved)
                {
                    const auto& locked = lockfile.packages.at(name);
                    if (locked.integrity.empty())
                    {
                        continue;  // Older lockfiles may pre-date integrity recording.
                    }
                    std::string actual = "sha256-" + Sha256::hashDirectory(pkg.registryPath);
                    if (actual != locked.integrity)
                    {
                        throw std::runtime_error(
                            "Integrity mismatch for " + name + "@" + pkg.version + "\n"
                            "  expected: " + locked.integrity + "\n"
                            "  actual:   " + actual + "\n"
                            "Pass --force-resolve to re-resolve and rewrite the lockfile.");
                    }
                }
            }
            else
            {
                reportProgress("Resolving dependencies...");
                DependencyResolver resolver(registry);
                resolved = resolver.resolve(dependencies);
            }

            // Ensure mt_modules/ exists
            fs::path modulesDir = fs::path(projectRoot) / "mt_modules";
            if (!fs::exists(modulesDir))
            {
                fs::create_directories(modulesDir);
            }

            // Install each resolved package
            for (const auto& [name, pkg] : resolved)
            {
                if (modulesManager.isInstalled(name))
                {
                    reportProgress("  Up to date: " + name + "@" + pkg.version);
                    ++result.upToDate;
                }
                else
                {
                    reportProgress("  Installing: " + name + "@" + pkg.version);
                    copyPackageToModules(pkg);
                    ++result.installed;
                }
            }

            // Remove packages not in resolved set
            auto installed = modulesManager.getInstalledPackages();
            for (const auto& pkgName : installed)
            {
                if (resolved.find(pkgName) == resolved.end())
                {
                    reportProgress("  Removing: " + pkgName);
                    modulesManager.removePackage(pkgName);
                    ++result.removed;
                }
            }

            // Write lockfile
            Lockfile newLockfile;
            for (const auto& [name, pkg] : resolved)
            {
                LockedPackage locked;
                locked.name = name;
                locked.version = pkg.version;
                locked.resolved = "file://" + pkg.registryPath;
                locked.integrity = "sha256-" + Sha256::hashDirectory(pkg.registryPath);
                locked.dependencies = pkg.dependencies;
                newLockfile.packages[name] = locked;
            }
            newLockfile.saveToFile(lockfilePath);
            reportProgress("Lockfile written: mtproj.lock");

            // Compile each package to .mtcLib in its registry version dir so
            // the build step downstream can link against compiled bytecode.
            // The compile pass is the bridge that closes MYT-323: mtpm installs
            // source; mType expects bytecode. Without this, ~/.mtype/registry
            // would only ever hold source trees and LibraryLinker would always
            // fail to resolve them.
            if (!resolved.empty())
            {
                if (mTypeExePath.empty())
                {
                    reportProgress(
                        "Warning: mType.exe not found alongside mtpm.exe — "
                        "skipping bytecode compile step. Builds against this "
                        "package will fail until mType is on the PATH or "
                        "co-located with mtpm.");
                }
                else
                {
                    PackageCompiler compiler;
                    compiler.setMTypeExecutable(mTypeExePath);
                    compiler.setProgressCallback(progressCallback);

                    reportProgress("Compiling packages to .mtcLib...");
                    auto compileResult = compiler.compileAll(resolved);
                    if (!compileResult.success)
                    {
                        for (const auto& e : compileResult.errors)
                        {
                            result.errors.push_back("compile: " + e);
                        }
                        // Surface as soft failure: registry source is still usable
                        // for tools that don't go through LibraryLinker (LSP via
                        // mt_modules), so don't roll back the lockfile.
                        result.success = false;
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            result.success = false;
            result.errors.push_back(e.what());
        }

        return result;
    }

    InstallResult PackageInstaller::addPackage(const std::string& name,
                                                const std::string& versionRange,
                                                std::vector<PackageDependency>& dependencies)
    {
        // Check if already in dependencies
        for (auto& dep : dependencies)
        {
            if (dep.name == name)
            {
                dep.versionRange = versionRange;
                return install(dependencies);
            }
        }

        dependencies.push_back({name, versionRange});
        return install(dependencies);
    }

    InstallResult PackageInstaller::removePackage(const std::string& name,
                                                   std::vector<PackageDependency>& dependencies)
    {
        // Remove from dependency list
        dependencies.erase(
            std::remove_if(dependencies.begin(), dependencies.end(),
                [&name](const PackageDependency& dep) { return dep.name == name; }),
            dependencies.end());

        // Re-install to clean up transitive deps
        auto result = install(dependencies);

        // Also explicitly remove the package
        if (modulesManager.isInstalled(name))
        {
            modulesManager.removePackage(name);
        }

        return result;
    }

    void PackageInstaller::copyPackageToModules(const ResolvedPackage& pkg)
    {
        fs::path source = fs::path(pkg.registryPath);
        fs::path dest = fs::path(projectRoot) / "mt_modules" / ("@" + pkg.name);

        if (fs::exists(dest))
        {
            fs::remove_all(dest);
        }

        fs::create_directories(dest);
        fs::copy(source, dest, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    }
}
