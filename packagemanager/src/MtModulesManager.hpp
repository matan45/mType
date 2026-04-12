#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace packagemanager
{
    class MtModulesManager
    {
    public:
        explicit MtModulesManager(const std::string& projectRoot);

        // Scan mt_modules/ and return alias map: @pkgName -> absolute source path
        std::unordered_map<std::string, std::string> getAliases() const;

        // Get list of installed package names
        std::vector<std::string> getInstalledPackages() const;

        // Get the mt_modules directory path
        std::string getMtModulesPath() const;

        // Check if a package is installed
        bool isInstalled(const std::string& packageName) const;

        // Remove a package from mt_modules/
        void removePackage(const std::string& packageName);

    private:
        std::string projectRoot;
    };
}
