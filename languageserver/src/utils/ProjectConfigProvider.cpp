#include "ProjectConfigProvider.hpp"
// MYT-309 — relative include so the build works even if the `packagemanager/src`
// includedir hasn't been re-emitted by premake yet. The header sits at
// packagemanager/src/, three levels up from languageserver/src/utils/.
#include "MtModulesManager.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>

namespace fs = std::filesystem;

namespace mtype::lsp {

bool ProjectConfigProvider::loadFromWorkspace(const std::string& workspaceRoot)
{
    loaded_ = false;
    searchPaths_.clear();
    aliases_.clear();
    projectRoot_.clear();
    workspaceRoot_ = workspaceRoot;

    std::string mtprojPath = findMtproj(workspaceRoot);
    if (mtprojPath.empty())
    {
        return false;
    }

    // MYT-309 — pin projectRoot_ before mt_modules scan so the merge step
    // and parseMtproj agree on the project root.
    projectRoot_ = fs::path(mtprojPath).parent_path().string();

    // mt_modules aliases populate first; <Alias> entries from parseMtproj
    // run last and override on collision via operator[] — same precedence
    // as ProjectBuilder::buildMergedAliases.
    mergeMtModulesAliases();

    parseMtproj(mtprojPath);
    return loaded_;
}

bool ProjectConfigProvider::reload()
{
    if (workspaceRoot_.empty()) return false;
    return loadFromWorkspace(workspaceRoot_);
}

bool ProjectConfigProvider::isWithinWorkspace(const std::string& candidatePath, const std::string& workspaceRoot) const
{
    try
    {
        auto canonical = fs::weakly_canonical(candidatePath).string();
        auto root = fs::weakly_canonical(workspaceRoot).string();
        return canonical.find(root) == 0;
    }
    catch (...)
    {
        return false;
    }
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
            // libstdc++/libc++ correctly return "" for path(".mtproj").extension()
            // (single leading dot = hidden file, no extension per C++17), while
            // MSVC's <filesystem> non-conformingly returns ".mtproj". Match both
            // the dotfile and the `Name.mtproj` form so all platforms agree.
            if (path.extension() == ".mtproj" || path.filename() == ".mtproj")
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
    catch (const std::exception& e)
    {
        std::cerr << "[mType LSP] Error searching for .mtproj: " << e.what() << std::endl;
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

        // projectRoot_ is pinned upfront in loadFromWorkspace so the
        // mt_modules merge and the XML pass share the same root.

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
                std::string normalized = absPath.lexically_normal().string();
                if (isWithinWorkspace(normalized, workspaceRoot_))
                {
                    searchPaths_.push_back(normalized);
                }
                else
                {
                    std::cerr << "[mType LSP] Ignoring search path outside workspace: " << normalized << std::endl;
                }
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
                std::string normalized = absPath.lexically_normal().string();
                if (isWithinWorkspace(normalized, workspaceRoot_))
                {
                    aliases_[aliasName] = normalized;
                }
                else
                {
                    std::cerr << "[mType LSP] Ignoring alias outside workspace: " << aliasName << " -> " << normalized << std::endl;
                }
            }
            ++it;
        }

        loaded_ = true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[mType LSP] Error parsing .mtproj: " << e.what() << std::endl;
        loaded_ = false;
    }
}

void ProjectConfigProvider::mergeMtModulesAliases()
{
    if (projectRoot_.empty())
    {
        return;
    }

    try
    {
        packagemanager::MtModulesManager modulesMgr(projectRoot_);
        for (const auto& [name, path] : modulesMgr.getAliases())
        {
            // SECURITY: mt_modules/ lives under projectRoot_/workspaceRoot_, but
            // a symlinked source dir could escape — gate every entry through the
            // same workspace-containment check used for <Alias> entries.
            if (!isWithinWorkspace(path, workspaceRoot_))
            {
                std::cerr << "[mType LSP] Ignoring mt_modules alias outside workspace: "
                          << name << " -> " << path << std::endl;
                continue;
            }
            aliases_[name] = path;
        }
    }
    catch (const std::exception& e)
    {
        // A corrupt mt_modules/ (permission denied, malformed mtpkg.json,
        // symlink loop) must not kill the rest of the project config — log
        // and continue without those aliases.
        std::cerr << "[mType LSP] Failed to scan mt_modules/: " << e.what() << std::endl;
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
