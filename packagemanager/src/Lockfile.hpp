#pragma once
#include <string>
#include <unordered_map>

namespace packagemanager
{
    struct LockedPackage
    {
        std::string name;
        std::string version;
        std::string resolved;
        std::string integrity;
        std::unordered_map<std::string, std::string> dependencies;
    };

    struct Lockfile
    {
        int version = 1;
        std::unordered_map<std::string, LockedPackage> packages;

        static Lockfile parseFromJson(const std::string& jsonString);

        static Lockfile loadFromFile(const std::string& filePath);

        std::string toJson() const;

        void saveToFile(const std::string& filePath) const;
    };
}
