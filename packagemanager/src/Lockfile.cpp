#include "Lockfile.hpp"
#include "../../mType/json/JsonParser.hpp"
#include "../../mType/json/JsonValue.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace packagemanager
{
    Lockfile Lockfile::parseFromJson(const std::string& jsonString)
    {
        auto root = json::JsonParser::parse(jsonString);

        if (!root || !root->isObject())
        {
            throw std::runtime_error("Invalid mtproj.lock: expected JSON object");
        }

        Lockfile lockfile;

        if (root->hasProperty("version") && root->getProperty("version")->isInt())
        {
            lockfile.version = static_cast<int>(root->getProperty("version")->asInt());
        }

        if (root->hasProperty("packages") && root->getProperty("packages")->isObject())
        {
            const auto& pkgs = root->getProperty("packages")->asObject();
            for (const auto& [name, value] : pkgs)
            {
                if (!value->isObject()) continue;

                LockedPackage pkg;
                pkg.name = name;

                if (value->hasProperty("version") && value->getProperty("version")->isString())
                {
                    pkg.version = value->getProperty("version")->asString();
                }

                if (value->hasProperty("resolved") && value->getProperty("resolved")->isString())
                {
                    pkg.resolved = value->getProperty("resolved")->asString();
                }

                if (value->hasProperty("integrity") && value->getProperty("integrity")->isString())
                {
                    pkg.integrity = value->getProperty("integrity")->asString();
                }

                if (value->hasProperty("dependencies") && value->getProperty("dependencies")->isObject())
                {
                    const auto& deps = value->getProperty("dependencies")->asObject();
                    for (const auto& [depName, depValue] : deps)
                    {
                        if (depValue->isString())
                        {
                            pkg.dependencies[depName] = depValue->asString();
                        }
                    }
                }

                lockfile.packages[name] = std::move(pkg);
            }
        }

        return lockfile;
    }

    Lockfile Lockfile::loadFromFile(const std::string& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open lockfile: " + filePath);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return parseFromJson(buffer.str());
    }

    std::string Lockfile::toJson() const
    {
        auto root = json::JsonValue::object();

        root->setProperty("version", json::JsonValue::integer(version));

        auto pkgs = json::JsonValue::object();
        for (const auto& [name, pkg] : packages)
        {
            auto pkgObj = json::JsonValue::object();
            pkgObj->setProperty("version", json::JsonValue::string(pkg.version));
            pkgObj->setProperty("resolved", json::JsonValue::string(pkg.resolved));
            pkgObj->setProperty("integrity", json::JsonValue::string(pkg.integrity));

            if (!pkg.dependencies.empty())
            {
                auto deps = json::JsonValue::object();
                for (const auto& [depName, depVersion] : pkg.dependencies)
                {
                    deps->setProperty(depName, json::JsonValue::string(depVersion));
                }
                pkgObj->setProperty("dependencies", deps);
            }

            pkgs->setProperty(name, pkgObj);
        }

        root->setProperty("packages", pkgs);

        return root->toJsonString(true);
    }

    void Lockfile::saveToFile(const std::string& filePath) const
    {
        std::ofstream file(filePath);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not write lockfile: " + filePath);
        }
        file << toJson();
    }
}
