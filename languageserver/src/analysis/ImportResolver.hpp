#pragma once

#include <optional>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include "../../../mType/environment/Environment.hpp"
#include "../DocumentManager.hpp"
#include "../utils/ProjectConfigProvider.hpp"
#include "ImportSummary.hpp"

namespace mtype::lsp {

// Forward declaration
struct Document;

/**
 * Resolves a document's imports, transitively, and registers the imported
 * symbols into the document's environment.
 *
 * Each imported file is parsed once and cached by canonical path. A cache
 * entry remembers the file's own resolved imports, so a cache hit still
 * walks the whole import graph; the compiler makes everything reachable
 * through imports visible, and so does this resolver (MYT-440). Every
 * depth resolves through the project config first (mt_modules aliases),
 * then relative to the importing file.
 */
class ImportResolver {
public:
    ImportResolver();

    /**
     * Set the project configuration for search path resolution
     */
    void setProjectConfig(std::shared_ptr<ProjectConfigProvider> config);

    /**
     * Resolve every import of `doc`, register the imported symbols into
     * `doc->environment` and `doc->symbolLocations`, and report what the
     * imports make visible.
     *
     * @param astIsCurrent true when `doc->ast` was produced from the
     *        current content. Otherwise the import list falls back to a
     *        line regex and the summary is marked incomplete.
     */
    ImportSummary resolveImports(Document* doc, bool astIsCurrent);

    /**
     * Drop every cached import. Call when `.mt` files or the project
     * configuration change on disk.
     */
    void clearCache();

    /** One import statement, as far as resolution cares. */
    struct ImportSpec {
        std::string path;                 // as written, e.g. "./shapes/Circle.mt"
        bool library = false;             // `import lib ...`: not visible to the LS
        std::vector<std::string> names;   // selected names plus their aliases
    };

private:
    /** A parsed import target, cached by canonical path. */
    struct FileEntry {
        std::shared_ptr<environment::Environment> env;
        std::unordered_map<std::string, SymbolLocationInfo> locations;
        std::vector<std::string> nestedPaths;          // its own imports, resolved
        std::unordered_set<std::string> typeNames;     // declared + selected + aliases
        bool selfComplete = true;                      // all its own imports resolved
    };

    /** Where merged symbols go: the importing document's state. */
    struct MergeTarget {
        std::shared_ptr<environment::Environment> env;
        std::unordered_map<std::string, SymbolLocationInfo>& locations;
        std::unordered_set<std::string>& typeNames;
    };

    bool mergeImport(const ImportSpec& spec, const std::string& fromPath,
                     MergeTarget& target, std::unordered_set<std::string>& visited);
    bool mergeFile(const std::string& path, MergeTarget& target,
                   std::unordered_set<std::string>& visited);
    const FileEntry* loadEntry(const std::string& path);
    std::optional<FileEntry> parseEntry(const std::string& path) const;
    std::string resolveTarget(const std::string& fromFilePath,
                              const std::string& importPath) const;

    // Canonical path -> parsed import file. Node-based, so pointers to
    // entries stay valid while new ones are inserted during a merge.
    std::unordered_map<std::string, FileEntry> cache_;

    // Project configuration for search path resolution
    std::shared_ptr<ProjectConfigProvider> projectConfig_;
};

} // namespace mtype::lsp
