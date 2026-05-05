#pragma once
#include <optional>
#include <cstddef>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace diagnostics
{
    /**
     * Process-wide cache of source-file lines, used by DiagnosticRenderer
     * to render code snippets without re-reading from disk.
     *
     * Producers:
     *   - The Lexer publishes split lines after parse-time tokenization.
     *   - The LSP DocumentManager publishes in-memory editor buffers, which
     *     may diverge from the on-disk file.
     *
     * Consumers:
     *   - DiagnosticRenderer queries by (filename, 1-based line number) to
     *     produce snippet lines for the caret/secondary spans.
     *
     * Filenames are normalized (forward slashes, no double-separators) so
     * the same file is not stored twice under "C:\foo" and "C:/foo". Disk
     * canonicalization is intentionally avoided here because LSP buffers
     * use logical URIs that may not exist on disk.
     *
     * The cache is thread-safe; reads use shared locking, writes use
     * exclusive locking.
     */
    class SourceFileCache
    {
    public:
        static SourceFileCache& instance();

        /**
         * Publish a vector of pre-split lines for a file. Overwrites any
         * existing entry. Used by the Lexer.
         */
        void publish(const std::string& filename, std::vector<std::string> lines);

        /**
         * Publish raw content; the cache splits on newlines internally.
         * Used by the LSP DocumentManager when an editor buffer is parsed.
         */
        void publishFromContent(const std::string& filename, const std::string& content);

        /**
         * Look up a single line by 1-based line number. Returns nullopt if
         * the file is not cached or the line number is out of range.
         */
        std::optional<std::string> getLine(const std::string& filename, int line1Based) const;

        /**
         * Total number of lines cached for a file, or 0 if not cached.
         */
        size_t getLineCount(const std::string& filename) const;

        /**
         * Drop a single file. Called by the LSP on document close.
         */
        void invalidate(const std::string& filename);

        /**
         * Drop everything. Used by tests.
         */
        void clear();

        /**
         * Normalize a filename for cache lookup (forward slashes, collapsed
         * separators). Exposed for tests.
         */
        static std::string normalizeFilename(const std::string& filename);

    private:
        SourceFileCache() = default;
        SourceFileCache(const SourceFileCache&) = delete;
        SourceFileCache& operator=(const SourceFileCache&) = delete;

        static std::vector<std::string> splitLines(const std::string& content);

        mutable std::shared_mutex mutex_;
        std::unordered_map<std::string, std::vector<std::string>> files_;
    };
}
