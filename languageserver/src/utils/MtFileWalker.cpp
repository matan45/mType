#include "MtFileWalker.hpp"

#include <algorithm>
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace mtype::lsp::utils
{
    namespace
    {
        bool shouldSkipDir(const std::string& name)
        {
            if (name.empty()) return false;
            if (name[0] == '.') return true;            // .git, .vscode, etc.
            if (name == "node_modules") return true;
            if (name == "bin") return true;
            if (name == "obj") return true;
            if (name == "build") return true;
            return false;
        }
    } // namespace

    std::vector<std::string> MtFileWalker::findMtFiles(const std::string& workspaceRoot)
    {
        std::vector<std::string> result;
        if (workspaceRoot.empty()) return result;

        std::error_code ec;
        if (!fs::exists(workspaceRoot, ec) || !fs::is_directory(workspaceRoot, ec))
        {
            return result;
        }

        // recursive_directory_iterator + manual prune. We can't easily
        // tell the iterator to skip a subtree, so we walk every entry
        // and skip files inside directories whose path component starts
        // with `.` or matches the build-dir blacklist.
        fs::recursive_directory_iterator it(
            workspaceRoot,
            fs::directory_options::skip_permission_denied,
            ec);
        if (ec) return result;

        const fs::recursive_directory_iterator end;
        for (; it != end; it.increment(ec))
        {
            if (ec)
            {
                ec.clear();
                continue;
            }

            const auto& entry = *it;

            if (entry.is_directory(ec))
            {
                if (shouldSkipDir(entry.path().filename().string()))
                {
                    it.disable_recursion_pending();
                }
                continue;
            }

            if (entry.is_regular_file(ec) && entry.path().extension() == ".mt")
            {
                result.push_back(entry.path().string());
            }
        }

        std::sort(result.begin(), result.end());
        return result;
    }

    bool MtFileWalker::isWorkspaceFile(const std::string& workspaceRoot,
                                       const std::string& filePath)
    {
        if (workspaceRoot.empty() || filePath.empty()) return false;

        std::error_code ec;
        const fs::path file = fs::weakly_canonical(fs::path(filePath), ec);
        if (ec || file.extension() != ".mt" || !fs::is_regular_file(file, ec)) return false;
        const fs::path root = fs::weakly_canonical(fs::path(workspaceRoot), ec);
        if (ec) return false;

        // Different drives (or no common prefix) give an empty relative path.
        const fs::path rel = file.lexically_relative(root);
        if (rel.empty()) return false;

        // Every directory between the root and the file is one the scan
        // would have descended into; one it skips means the scan never
        // indexed this file.
        for (const auto& dir : rel.parent_path())
        {
            const std::string part = dir.string();
            if (part == ".." || shouldSkipDir(part)) return false;
        }
        return true;
    }
}
