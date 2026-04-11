#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace mtype::lsp
{
    class ProjectConfigProvider;
}

namespace mtype::lsp::analysis
{
    /**
     * A single top-level symbol discovered in a workspace `.mt` file.
     * Mirrors `SymbolLocationInfo` but adds a kind tag and stores the
     * file URI in fully-qualified form so the missing-import quick fix
     * can route across files.
     */
    enum class WorkspaceSymbolKind
    {
        Unknown,
        Class,
        Function,
        Interface
    };

    struct WorkspaceSymbol
    {
        std::string name;       // top-level only ("Foo", "bar"); members ("Foo.method") are filtered out
        std::string fileUri;    // file:// URI
        int line = 0;           // 0-based
        int column = 0;         // 0-based
        WorkspaceSymbolKind kind = WorkspaceSymbolKind::Unknown;
    };

    /**
     * Workspace-wide name → declaring file index used to power
     * MYT-47's "Add import 'Foo'" quick fix.
     *
     * Built once at LSP `initialize` (off-thread), then refreshed per
     * file via `reindexFile` whenever a document is reparsed. Reads
     * happen on the main request thread; the internal mutex covers
     * the read/write race.
     *
     * Top-level only — qualified member names ("Foo.method") are
     * skipped because the missing-import fix only operates on
     * file-level declarations.
     */
    class WorkspaceSymbolIndex
    {
    public:
        WorkspaceSymbolIndex() = default;
        ~WorkspaceSymbolIndex() = default;

        /**
         * Initial scan. Walks the workspace recursively, parses every
         * `.mt` file, runs symbol registration, and populates the
         * `byName_` map. Safe to call from any thread; the LSP
         * initialise handler kicks this off via `std::async` to avoid
         * blocking the initialise response on a large workspace.
         */
        void buildFromWorkspace(const std::string& workspaceRoot);

        /**
         * Refresh the index entries for one file. Drops the file's
         * existing entries first, then re-parses (cheap if the LSP just
         * parsed it for diagnostics). Called from
         * `DocumentManager::parseDocument` after a successful reparse.
         */
        void reindexFile(const std::string& fileUri);

        /**
         * Drop every entry whose `fileUri` matches. Called when a
         * document is closed or deleted from disk.
         */
        void invalidateFile(const std::string& fileUri);

        /**
         * Top-level lookup. Returns at most `maxResults` matches; the
         * order is alphabetic by `fileUri` for determinism.
         */
        std::vector<WorkspaceSymbol> findByName(
            const std::string& name,
            std::size_t maxResults = 5) const;

        /**
         * Convert an absolute file path to a relative import spelling
         * usable in an `import ... from "..."` statement, computed from
         * the path of the file that needs the import. The result has
         * forward slashes and no `.mt` extension. Returns the original
         * path on failure.
         */
        static std::string computeImportSpelling(
            const std::string& symbolFilePath,
            const std::string& referencingFilePath);

    private:
        // Hot path is read-heavy after the initial build. shared_mutex
        // would be ideal but std::shared_mutex isn't free on Windows;
        // a plain mutex is fine because reads are short.
        mutable std::mutex mutex_;
        std::unordered_map<std::string, std::vector<WorkspaceSymbol>> byName_;
        std::unordered_map<std::string, std::vector<std::string>> byFile_; // fileUri → names

        // Indexes a single file. Caller may hold the mutex; this method
        // does not lock it itself.
        void indexFileLocked(const std::string& filePath);
    };
}
