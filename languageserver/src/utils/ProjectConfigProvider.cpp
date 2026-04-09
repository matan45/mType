#include "ProjectConfigProvider.hpp"
#include <fstream>
#include <sstream>
#include <regex>

namespace fs = std::filesystem;

namespace mtype::lsp {

bool ProjectConfigProvider::loadFromWorkspace(const std::string& workspaceRoot)
{
    loaded_ = false;
    searchPaths_.clear();
    aliases_.clear();
    projectRoot_.clear();

    std::string mtprojPath = findMtproj(workspaceRoot);
    if (mtprojPath.empty())
    {
        return false;
    }

    parseMtproj(mtprojPath);
    return loaded_;
}

std::string ProjectConfigProvider::findMtproj(const std::string& startDir) const
{
    try
    {
        // Search recursively for .mtproj files, prefer the one closest to root (shortest path)
        std::string bestPath;

        for (const auto& entry : fs::recursive_directory_iterator(
            startDir, fs::directory_options::skip_permission_denied))
        {
            if (!entry.is_regular_file()) continue;

            const auto& path = entry.path();
            if (path.extension() == ".mtproj")
            {
                std::string candidate = path.string();
                if (bestPath.empty() || candidate.length() < bestPath.length())
                {
                    bestPath = candidate;
                }
            }
        }

        return bestPath;
    }
    catch (...)
    {
        return "";
    }
}

void ProjectConfigProvider::parseMtproj(const std::string& filePath)
{
    try
    {
        std::ifstream file(filePath);
        if (!file.is_open()) return;

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        projectRoot_ = fs::path(filePath).parent_path().string();

        // Extract <SearchPath> entries
        std::regex searchPathRegex("<SearchPath>(.*?)</SearchPath>");
        std::sregex_iterator it(content.begin(), content.end(), searchPathRegex);
        std::sregex_iterator end;

        while (it != end)
        {
            std::string relativePath = (*it)[1].str();
            if (!relativePath.empty())
            {
                fs::path absPath = fs::path(projectRoot_) / relativePath;
                searchPaths_.push_back(absPath.lexically_normal().string());
            }
            ++it;
        }

        // Extract <Alias Name="@name" Path="path" /> entries
        std::regex aliasRegex("<Alias\\s+Name=\"([^\"]+)\"\\s+Path=\"([^\"]+)\"\\s*/>");
        it = std::sregex_iterator(content.begin(), content.end(), aliasRegex);

        while (it != end)
        {
            std::string aliasName = (*it)[1].str();
            std::string aliasPath = (*it)[2].str();
            if (!aliasName.empty() && !aliasPath.empty())
            {
                fs::path absPath = fs::path(projectRoot_) / aliasPath;
                aliases_[aliasName] = absPath.lexically_normal().string();
            }
            ++it;
        }

        loaded_ = true;
    }
    catch (...)
    {
        loaded_ = false;
    }
}

std::string ProjectConfigProvider::resolveImport(const std::string& baseDir, const std::string& importPath) const
{
    std::string resolvedPath = importPath;

    // 1. Alias expansion
    if (!resolvedPath.empty() && resolvedPath[0] == '@')
    {
        size_t slashPos = resolvedPath.find('/');
        if (slashPos == std::string::npos) slashPos = resolvedPath.find('\\');

        std::string aliasName = (slashPos != std::string::npos)
            ? resolvedPath.substr(0, slashPos)
            : resolvedPath;

        auto it = aliases_.find(aliasName);
        if (it != aliases_.end())
        {
            std::string remainder = (slashPos != std::string::npos)
                ? resolvedPath.substr(slashPos)
                : "";
            resolvedPath = it->second + remainder;
        }
    }

    fs::path filePath(resolvedPath);

    // 2. If already absolute, return if exists
    if (!filePath.is_relative())
    {
        auto normalized = filePath.lexically_normal();
        if (fs::exists(normalized)) return normalized.string();
        return "";
    }

    // 3. Try relative to current file's directory
    {
        fs::path candidate = fs::path(baseDir) / filePath;
        candidate = candidate.lexically_normal();
        if (fs::exists(candidate)) return candidate.string();
    }

    // 4. Try relative to project root
    if (!projectRoot_.empty())
    {
        fs::path candidate = fs::path(projectRoot_) / filePath;
        candidate = candidate.lexically_normal();
        if (fs::exists(candidate)) return candidate.string();
    }

    // 5. Try each search path
    for (const auto& searchPath : searchPaths_)
    {
        fs::path candidate = fs::path(searchPath) / filePath;
        candidate = candidate.lexically_normal();
        if (fs::exists(candidate)) return candidate.string();
    }

    return "";
}

} // namespace mtype::lsp
