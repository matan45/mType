#include "MtModulesManager.hpp"
#include "PackageManifest.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace packagemanager
{
    namespace fs = std::filesystem;

    MtModulesManager::MtModulesManager(const std::string& projectRoot)
        : projectRoot(projectRoot)
    {
    }

    std::string MtModulesManager::getMtModulesPath() const
    {
        return (fs::path(projectRoot) / "mt_modules").string();
    }

    std::unordered_map<std::string, std::string> MtModulesManager::getAliases() const
    {
        std::unordered_map<std::string, std::string> aliases;
        fs::path modulesDir = fs::path(projectRoot) / "mt_modules";

        if (!fs::exists(modulesDir) || !fs::is_directory(modulesDir))
        {
            return aliases;
        }

        for (const auto& entry : fs::directory_iterator(modulesDir))
        {
            if (!entry.is_directory()) continue;

            std::string dirName = entry.path().filename().string();

            // Package directories start with @
            if (dirName.empty() || dirName[0] != '@') continue;

            // Check for mtpkg.json to confirm it's a valid package
            fs::path manifestPath = entry.path() / "mtpkg.json";
            if (!fs::exists(manifestPath)) continue;

            // Read manifest to get the source directory
            try
            {
                std::ifstream file(manifestPath);
                std::stringstream buffer;
                buffer << file.rdbuf();

                PackageManifest manifest = PackageManifest::parseFromJson(buffer.str());

                // Register alias: @pkgName -> absolute path to source dir
                fs::path sourcePath = entry.path() / manifest.sourceDir;
                if (fs::exists(sourcePath))
                {
                    aliases[dirName] = fs::canonical(sourcePath).string();
                }
                else
                {
                    // Fall back to the package root
                    aliases[dirName] = fs::canonical(entry.path()).string();
                }
            }
            catch (const std::exception&)
            {
                // Skip packages with invalid manifests
            }
        }

        return aliases;
    }

    std::vector<std::string> MtModulesManager::getInstalledPackages() const
    {
        std::vector<std::string> packages;
        fs::path modulesDir = fs::path(projectRoot) / "mt_modules";

        if (!fs::exists(modulesDir)) return packages;

        for (const auto& entry : fs::directory_iterator(modulesDir))
        {
            if (!entry.is_directory()) continue;

            std::string dirName = entry.path().filename().string();
            if (!dirName.empty() && dirName[0] == '@')
            {
                fs::path manifestPath = entry.path() / "mtpkg.json";
                if (fs::exists(manifestPath))
                {
                    packages.push_back(dirName.substr(1)); // Remove @ prefix
                }
            }
        }

        return packages;
    }

    bool MtModulesManager::isInstalled(const std::string& packageName) const
    {
        fs::path pkgDir = fs::path(projectRoot) / "mt_modules" / ("@" + packageName);
        return fs::exists(pkgDir / "mtpkg.json");
    }

    void MtModulesManager::removePackage(const std::string& packageName)
    {
        fs::path pkgDir = fs::path(projectRoot) / "mt_modules" / ("@" + packageName);
        if (fs::exists(pkgDir))
        {
            fs::remove_all(pkgDir);
        }
    }
}
