#pragma once

#include "WorkspaceSymbol.hpp"
#include "WorkspaceTypeGraph.hpp"

#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mtype::lsp
{
    class ProjectConfigProvider;
}

namespace mtype::lsp::analysis
{
    /**
     * Workspace-wide index of every `.mt` file, open or closed. Powers
     * MYT-47's "Add import 'Foo'" quick fix, auto-import completion,
     * Ctrl+T, and (MYT-432) the type graph behind implementation and
     * type-hierarchy queries.
     *
     * Built once at LSP `initialize` (off-thread), then refreshed per
     * file. Each file is parsed once (WorkspaceFileParser) and that parse
     * feeds both the top-level symbol tables and `typeGraph()`. Reads
     * happen on the main request thread; the internal mutex covers the
     * read/write race.
     *
     * Freshness rules (MYT-432 / MYT-426):
     * - A file's entries are replaced only after a successful parse, so a
     *   parse error keeps the last good entries.
     * - Every entry point keys files by `UriUtils::canonicalFileUri`, so
     *   different client spellings of one file never duplicate entries.
     * - A file opened through `openFile` is owned by its editor buffer
     *   until `revertToDisk`: disk events and the workspace scan skip it.
     *
     * Symbols are top-level only — qualified member names ("Foo.method")
     * are skipped because the missing-import fix only operates on
     * file-level declarations. Members live in the type graph.
     */
    class WorkspaceSymbolIndex
    {
    public:
        WorkspaceSymbolIndex() = default;
        ~WorkspaceSymbolIndex() = default;

        /**
         * Initial scan. Walks the workspace recursively, parses every
         * `.mt` file, and indexes it. Files open in the editor keep their
         * buffer's entries. Takes the lock once per file, so readers and
         * didOpen are not blocked for the whole scan. Safe to call from
         * any thread; the LSP initialise handler kicks this off via
         * `std::async` to avoid blocking the initialise response on a
         * large workspace.
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
         * The editor opened `fileUri` (didOpen). Indexes the buffer and
         * marks the file as owned by it, so disk events and the workspace
         * scan leave it alone until `revertToDisk` (didClose).
         */
        void openFile(const std::string& fileUri, const std::string& content);

        /**
         * Refresh one file from disk — a file-system event for a document
         * the editor doesn't have open. Skipped for a file owned by an open
         * buffer: the buffer takes precedence, and closing it re-reads disk.
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
         * The file was deleted from disk. Drops its entries, unless an open
         * buffer owns it: the buffer still exists in the editor, and
         * closing it drops the file then.
         */
        void invalidateFile(const std::string& fileUri);

        /**
         * Called when the editor closes a document (MYT-447). The entries
         * may describe the unsaved buffer, so they are rebuilt from the file
         * on disk — the file still exists, and dropping it hid its symbols
         * from Ctrl+T and Add import. A file the workspace scan would not
         * have indexed (outside the root, in a skipped folder, or deleted)
         * is dropped instead. Releases the buffer's ownership either way; if
         * the disk version doesn't parse, the last good entries stay.
         */
        void revertToDisk(const std::string& fileUri);

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
         * An empty prefix returns nothing (auto-import completion relies
         * on that); use `listAll` to enumerate.
         */
        std::vector<WorkspaceSymbol> findByPrefix(
            const std::string& prefix,
            std::size_t maxResults = 20) const;

        /**
         * Every indexed top-level symbol, ranked the same way as
         * `findByPrefix` (alphabetical by lowercased name, then spelling,
         * then file URI) and capped at `maxResults`. Powers the empty-query
         * case of `workspace/symbol` (MYT-447).
         */
        std::vector<WorkspaceSymbol> listAll(std::size_t maxResults) const;

        /**
         * Return the URIs of all files currently in the index.
         * Used by the references handler to search across the workspace.
         */
        std::vector<std::string> getAllIndexedFiles() const;

        /**
         * MYT-432 — supertype/subtype edges and member locations for every
         * indexed file. Self-synchronized; kept in step with the symbol
         * tables by the same per-file updates.
         */
        const WorkspaceTypeGraph& typeGraph() const { return typeGraph_; }

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
        // a plain mutex is fine because reads are short. Lock order is
        // always mutex_ → typeGraph_'s own mutex.
        mutable std::mutex mutex_;
        std::unordered_map<std::string, std::vector<WorkspaceSymbol>> byName_;
        std::unordered_map<std::string, std::vector<std::string>> byFile_; // fileUri → names
        std::unordered_set<std::string> indexedFiles_; // every key with committed entries
        std::unordered_set<std::string> bufferOwned_;  // keys opened by the editor (openFile)
        WorkspaceTypeGraph typeGraph_;
        std::string root_; // workspace root given to buildFromWorkspace; empty until then

        // Future of the initial workspace scan, set by setReadyFuture
        // during LSP initialise. waitForReady consults it to short-block
        // early code-action / completion requests until the scan is
        // populated. Mutable so a const waitForReady can call wait_for.
        mutable std::shared_future<void> readyFuture_;

        // Parses `content` and, only if that succeeds, replaces the file's
        // symbols and type facts. `key` is a canonical file URI. Caller
        // must hold the mutex.
        void indexContentLocked(const std::string& key, const std::string& content);

        // Drops everything the file contributed. Caller must hold the mutex.
        void dropFileLocked(const std::string& key);

        // Symbol-table halves of the two operations above.
        void dropSymbolsLocked(const std::string& key);
        void addSymbolsLocked(const std::string& key, std::vector<WorkspaceSymbol> symbols);
    };
}
