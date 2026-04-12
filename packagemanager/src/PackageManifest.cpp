#include "PackageManifest.hpp"
#include "../../mType/json/JsonParser.hpp"
#include "../../mType/json/JsonValue.hpp"
#include <stdexcept>

namespace packagemanager
{
    PackageManifest PackageManifest::parseFromJson(const std::string& jsonString)
    {
        auto root = json::JsonParser::parse(jsonString);

        if (!root || !root->isObject())
        {
            throw std::runtime_error("Invalid mtpkg.json: expected JSON object");
        }

        PackageManifest manifest;

        if (root->hasProperty("name") && root->getProperty("name")->isString())
        {
            manifest.name = root->getProperty("name")->asString();
        }
        else
        {
            throw std::runtime_error("mtpkg.json: 'name' field is required");
        }

        if (root->hasProperty("version") && root->getProperty("version")->isString())
        {
            manifest.version = root->getProperty("version")->asString();
        }
        else
        {
            throw std::runtime_error("mtpkg.json: 'version' field is required");
        }

        if (root->hasProperty("description") && root->getProperty("description")->isString())
        {
            manifest.description = root->getProperty("description")->asString();
        }

        if (root->hasProperty("source") && root->getProperty("source")->isString())
        {
            manifest.sourceDir = root->getProperty("source")->asString();
        }
        else
        {
            manifest.sourceDir = "src";
        }

        if (root->hasProperty("library") && root->getProperty("library")->isString())
        {
            manifest.libraryPath = root->getProperty("library")->asString();
        }

        if (root->hasProperty("dependencies") && root->getProperty("dependencies")->isObject())
        {
            const auto& deps = root->getProperty("dependencies")->asObject();
            for (const auto& [name, value] : deps)
            {
                if (value->isString())
                {
                    manifest.dependencies[name] = value->asString();
                }
            }
        }

        return manifest;
    }

    std::string PackageManifest::toJson() const
    {
        auto root = json::JsonValue::object();

        root->setProperty("name", json::JsonValue::string(name));
        root->setProperty("version", json::JsonValue::string(version));

        if (!description.empty())
        {
            root->setProperty("description", json::JsonValue::string(description));
        }

        root->setProperty("source", json::JsonValue::string(sourceDir));

        if (!libraryPath.empty())
        {
            root->setProperty("library", json::JsonValue::string(libraryPath));
        }

        if (!dependencies.empty())
        {
            auto deps = json::JsonValue::object();
            for (const auto& [name, version] : dependencies)
            {
                deps->setProperty(name, json::JsonValue::string(version));
            }
            root->setProperty("dependencies", deps);
        }

        return root->toJsonString(true);
    }
}
