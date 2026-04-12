#pragma once
#include <string>
#include <unordered_map>

namespace packagemanager
{
    struct PackageManifest
    {
        std::string name;
        std::string version;
        std::string description;
        std::string sourceDir;
        std::string libraryPath;
        std::unordered_map<std::string, std::string> dependencies;

        static PackageManifest parseFromJson(const std::string& jsonString);

        std::string toJson() const;
    };
}
