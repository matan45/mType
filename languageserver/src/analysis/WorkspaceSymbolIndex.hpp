#pragma once

#include <chrono>
#include <future>
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
         * Hand the index the future returned by the std::async that
         * runs `buildFromWorkspace`. The index doesn't own the build
         * task itself — the LSP server does — but it needs the future
         * so consumers can call `waitForReady()` without poking at the
         * server's private members.
         *
         * Call once during LSP initialise. Calling more than once
         * replaces the previous future.
         */
        void setReadyFuture(std::shared_future<void> future);

        /**
         * Wait up to `timeout` for the initial workspace scan to
         * complete. Returns true if the scan is done (or no scan was
         * scheduled), false if the timeout expired.
         *
         * Consumers (the missing-import quick fix and the auto-import
         * completion branch) call this before querying `findByName` so
         * an early request doesn't see an empty index. The 50 ms cap is
         * a sensible UX ceiling: if the scan takes longer, fall through
         * to "no suggestion" rather than blocking the user.
         */
        bool waitForReady(std::chrono::milliseconds timeout) const;

        /**
         * Refresh the index entries for one file. Drops the file's
         * existing entries first, then re-parses from disk. Used by the
         * initial build path and as a fallback when no in-memory buffer
         * is available (e.g., a file the editor hasn't opened yet).
         */
        void reindexFile(const std::string& fileUri);

        /**
         * Refresh the index entries for one file using the supplied
         * content directly (no disk I/O). Preferred path when the LSP
         * has the unsaved editor buffer in hand — without this overload,
         * symbols defined in unsaved files would not show up in the
         * auto-import quick fix until the user saved.
         */
        void reindexFile(const std::string& fileUri, const std::string& content);

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
         * Top-level prefix lookup for completion. Matches names
         * case-insensitively but returns original symbol spelling.
         */
        std::vector<WorkspaceSymbol> findByPrefix(
            const std::string& prefix,
            std::size_t maxResults = 20) const;

        /**
         * Return the URIs of all files currently in the index.
         * Used by the references handler to search across the workspace.
         */
        std::vector<std::string> getAllIndexedFiles() const;

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

        // Future of the initial workspace scan, set by setReadyFuture
        // during LSP initialise. waitForReady consults it to short-block
        // early code-action / completion requests until the scan is
        // populated. Mutable so a const waitForReady can call wait_for.
        mutable std::shared_future<void> readyFuture_;

        // Indexes a single file using the provided content. Caller must
        // hold the mutex; this method does not lock it itself.
        void indexFileLocked(const std::string& filePath, const std::string& content);

        // Drops every entry whose fileUri == `fileUri`. Caller must hold
        // the mutex. Shared by reindexFile and invalidateFile.
        void dropEntriesLocked(const std::string& fileUri);
    };
}
