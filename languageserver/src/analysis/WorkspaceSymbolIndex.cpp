#include "WorkspaceSymbolIndex.hpp"

#include "WorkspaceFileParser.hpp"
#include "../utils/MtFileWalker.hpp"
#include "../utils/StringUtils.hpp"
#include "../utils/UriUtils.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <utility>

namespace fs = std::filesystem;

namespace mtype::lsp::analysis
{
    namespace
    {
        // toLowerAscii lives in utils/StringUtils.hpp — shared with the
        // completion handler so both call sites have one definition.
        using mtype::lsp::utils::toLowerAscii;

        bool startsWithCaseInsensitive(const std::string& text,
                                       const std::string& prefix)
        {
            if (prefix.size() > text.size()) return false;
            for (std::size_t i = 0; i < prefix.size(); ++i)
            {
                const auto a = static_cast<unsigned char>(text[i]);
                const auto b = static_cast<unsigned char>(prefix[i]);
                if (std::tolower(a) != std::tolower(b)) return false;
            }
            return true;
        }

        // Orders `results` best-first and keeps at most `maxResults`: an
        // exact (case-insensitive) match of `loweredQuery` first, then by
        // lowercased name, original spelling and file URI so the output is
        // deterministic. Shared by findByPrefix and listAll so both rank the
        // same way. Runs on the caller's copy, outside the index mutex.
        std::vector<WorkspaceSymbol> rankAndTake(std::vector<WorkspaceSymbol> results,
                                                 const std::string& loweredQuery,
                                                 std::size_t maxResults)
        {
            // Pre-compute lowered names once so the comparator reduces to
            // cached-string comparisons instead of re-lowering both sides
            // on every call.
            struct Ranked {
                std::size_t idx;
                std::string lowered;
            };
            std::vector<Ranked> ranked;
            ranked.reserve(results.size());
            for (std::size_t i = 0; i < results.size(); ++i)
            {
                ranked.push_back({ i, toLowerAscii(results[i].name) });
            }

            const auto better = [&loweredQuery, &results](const Ranked& a, const Ranked& b)
            {
                const bool aExact = a.lowered == loweredQuery;
                const bool bExact = b.lowered == loweredQuery;
                if (aExact != bExact) return aExact;
                if (a.lowered != b.lowered) return a.lowered < b.lowered;
                const auto& sa = results[a.idx];
                const auto& sb = results[b.idx];
                if (sa.name != sb.name) return sa.name < sb.name;
                return sa.fileUri < sb.fileUri;
            };

            const std::size_t keep = std::min(ranked.size(), maxResults);
            std::partial_sort(ranked.begin(),
                              ranked.begin() + static_cast<std::ptrdiff_t>(keep),
                              ranked.end(), better);

            std::vector<WorkspaceSymbol> sorted;
            sorted.reserve(keep);
            for (std::size_t i = 0; i < keep; ++i)
            {
                sorted.push_back(std::move(results[ranked[i].idx]));
            }
            return sorted;
        }

        // Reads a file from disk into a string. Returns false on failure
        // (caller decides whether to silently skip or report).
        bool readFileFromDisk(const std::string& filePath, std::string& outContent)
        {
            std::ifstream stream(filePath);
            if (!stream.is_open()) return false;
            outContent.assign(
                (std::istreambuf_iterator<char>(stream)),
                std::istreambuf_iterator<char>());
            return true;
        }
    } // namespace

    void WorkspaceSymbolIndex::setReadyFuture(std::shared_future<void> future)
    {
        readyFuture_ = std::move(future);
    }

    bool WorkspaceSymbolIndex::waitForReady(std::chrono::milliseconds timeout) const
    {
        if (!readyFuture_.valid())
        {
            // No async build was ever scheduled — treat as ready (the
            // index might be empty, but we won't make consumers block
            // forever waiting for a future that doesn't exist).
            return true;
        }
        return readyFuture_.wait_for(timeout) == std::future_status::ready;
    }

    void WorkspaceSymbolIndex::buildFromWorkspace(const std::string& workspaceRoot)
    {
        const auto files = utils::MtFileWalker::findMtFiles(workspaceRoot);

        {
            // A (re)scan starts from scratch, except for files open in the
            // editor: their buffers take precedence over disk (MYT-432).
            std::lock_guard<std::mutex> lock(mutex_);
            root_ = workspaceRoot;
            std::vector<std::string> stale;
            for (const auto& key : indexedFiles_)
            {
                if (!bufferOwned_.count(key)) stale.push_back(key);
            }
            for (const auto& key : stale) dropFileLocked(key);
        }

        // One file per lock, so a didOpen or a read arriving mid-scan waits
        // for one file, not the whole workspace.
        for (const auto& path : files)
        {
            std::string content;
            if (!readFileFromDisk(path, content)) continue;

            const std::string key = UriUtils::filePathToUri(path);
            std::lock_guard<std::mutex> lock(mutex_);
            if (bufferOwned_.count(key)) continue;
            indexContentLocked(key, content);
        }
    }

    void WorkspaceSymbolIndex::openFile(const std::string& fileUri, const std::string& content)
    {
        const std::string key = UriUtils::canonicalFileUri(fileUri);
        std::lock_guard<std::mutex> lock(mutex_);
        bufferOwned_.insert(key);
        indexContentLocked(key, content);
    }

    void WorkspaceSymbolIndex::reindexFile(const std::string& fileUri)
    {
        // Disk path: a file-system event for a document the editor doesn't
        // own. The read happens outside the lock.
        const std::string key = UriUtils::canonicalFileUri(fileUri);
        std::string content;
        if (!readFileFromDisk(UriUtils::uriToFilePath(key), content)) return;

        std::lock_guard<std::mutex> lock(mutex_);
        if (bufferOwned_.count(key)) return; // the open buffer wins over disk
        indexContentLocked(key, content);
    }

    void WorkspaceSymbolIndex::reindexFile(const std::string& fileUri,
                                           const std::string& content)
    {
        // Buffer path. Preferred when the LSP has the live editor content
        // — without this, the auto-import quick fix would not see symbols
        // declared in unsaved files until they were saved.
        const std::string key = UriUtils::canonicalFileUri(fileUri);
        std::lock_guard<std::mutex> lock(mutex_);
        indexContentLocked(key, content);
    }

    void WorkspaceSymbolIndex::invalidateFile(const std::string& fileUri)
    {
        const std::string key = UriUtils::canonicalFileUri(fileUri);
        std::lock_guard<std::mutex> lock(mutex_);
        // An open buffer outlives its file on disk; closing it drops it.
        if (bufferOwned_.count(key)) return;
        dropFileLocked(key);
    }

    void WorkspaceSymbolIndex::revertToDisk(const std::string& fileUri)
    {
        const std::string key = UriUtils::canonicalFileUri(fileUri);
        std::string root;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            root = root_;
        }

        // Disk I/O happens outside the lock. A file the initial scan would
        // not have picked up (outside the workspace, in a skipped folder, or
        // deleted) is dropped instead, so it doesn't linger in Ctrl+T.
        const std::string path = UriUtils::uriToFilePath(key);
        std::string content;
        const bool keep = utils::MtFileWalker::isWorkspaceFile(root, path)
                       && readFileFromDisk(path, content);

        std::lock_guard<std::mutex> lock(mutex_);
        bufferOwned_.erase(key);
        if (keep)
        {
            indexContentLocked(key, content); // a disk parse error keeps the last good entries
        }
        else
        {
            dropFileLocked(key);
        }
    }

    std::vector<std::string> WorkspaceSymbolIndex::getAllIndexedFiles() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> files;
        files.reserve(byFile_.size());
        for (const auto& [uri, _] : byFile_) {
            files.push_back(uri);
        }
        return files;
    }

    void WorkspaceSymbolIndex::indexContentLocked(const std::string& key,
                                                  const std::string& content)
    {
        auto parsed = WorkspaceFileParser::parse(key, content);
        if (!parsed) return; // parse error: keep the file's last good entries

        dropSymbolsLocked(key);
        addSymbolsLocked(key, std::move(parsed->symbols));
        typeGraph_.replaceFile(key, std::move(parsed->types));
        indexedFiles_.insert(key);
    }

    void WorkspaceSymbolIndex::dropFileLocked(const std::string& key)
    {
        dropSymbolsLocked(key);
        typeGraph_.removeFile(key);
        indexedFiles_.erase(key);
    }

    void WorkspaceSymbolIndex::addSymbolsLocked(const std::string& key,
                                                std::vector<WorkspaceSymbol> symbols)
    {
        if (symbols.empty()) return;

        std::vector<std::string> names;
        names.reserve(symbols.size());
        for (auto& sym : symbols)
        {
            names.push_back(sym.name);
            byName_[sym.name].push_back(std::move(sym));
        }
        byFile_[key] = std::move(names);
    }

    void WorkspaceSymbolIndex::dropSymbolsLocked(const std::string& key)
    {
        auto byFileIt = byFile_.find(key);
        if (byFileIt == byFile_.end()) return;

        for (const auto& name : byFileIt->second)
        {
            auto byNameIt = byName_.find(name);
            if (byNameIt == byName_.end()) continue;
            auto& vec = byNameIt->second;
            vec.erase(
                std::remove_if(vec.begin(), vec.end(),
                    [&key](const WorkspaceSymbol& s) {
                        return s.fileUri == key;
                    }),
                vec.end());
            if (vec.empty()) byName_.erase(byNameIt);
        }
        byFile_.erase(byFileIt);
    }

    std::vector<WorkspaceSymbol> WorkspaceSymbolIndex::findByName(
        const std::string& name, std::size_t maxResults) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = byName_.find(name);
        if (it == byName_.end()) return {};

        std::vector<WorkspaceSymbol> results = it->second;
        if (results.size() > maxResults)
        {
            results.resize(maxResults);
        }
        return results;
    }

    std::vector<WorkspaceSymbol> WorkspaceSymbolIndex::findByPrefix(
        const std::string& prefix, std::size_t maxResults) const
    {
        if (prefix.empty() || maxResults == 0) return {};

        // Hold the mutex only long enough to copy the matching entries
        // into a local vector. The sort below operates on the copy,
        // so other readers and writers can proceed in parallel while
        // we rank.
        std::vector<WorkspaceSymbol> results;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& [name, symbols] : byName_)
            {
                if (!startsWithCaseInsensitive(name, prefix)) continue;
                results.insert(results.end(), symbols.begin(), symbols.end());
            }
        }

        if (results.empty()) return results;
        return rankAndTake(std::move(results), toLowerAscii(prefix), maxResults);
    }

    std::vector<WorkspaceSymbol> WorkspaceSymbolIndex::listAll(std::size_t maxResults) const
    {
        if (maxResults == 0) return {};

        std::vector<WorkspaceSymbol> results;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& [name, symbols] : byName_)
            {
                results.insert(results.end(), symbols.begin(), symbols.end());
            }
        }
        // No query, so no exact-match tier: the empty string never equals
        // a real symbol name and ranking falls through to alphabetical.
        return rankAndTake(std::move(results), std::string{}, maxResults);
    }

    std::string WorkspaceSymbolIndex::computeImportSpelling(
        const std::string& symbolFilePath,
        const std::string& referencingFilePath)
    {
        try
        {
            fs::path target(symbolFilePath);
            fs::path source(referencingFilePath);
            fs::path sourceDir = source.parent_path();

            // Compute relative path from the source file's directory.
            std::error_code ec;
            fs::path rel = fs::relative(target, sourceDir, ec);
            if (ec || rel.empty())
            {
                return symbolFilePath;
            }

            // mType requires the explicit `.mt` suffix in import paths
            // (the parser rejects extensionless imports — see test
            // suites under tests/testFiles/import/error/). If somehow
            // the symbol path didn't carry it, append.
            std::string spelling = rel.generic_string();
            if (spelling.size() < 3
                || spelling.compare(spelling.size() - 3, 3, ".mt") != 0)
            {
                spelling += ".mt";
            }

            // Ensure relative imports start with `./` so the parser
            // recognises them as relative.
            if (!spelling.empty() && spelling[0] != '.' && spelling[0] != '/')
            {
                spelling = "./" + spelling;
            }
            return spelling;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[mType LSP] computeImportSpelling failed for "
                      << symbolFilePath << " from " << referencingFilePath
                      << ": " << e.what() << std::endl;
            return symbolFilePath;
        }
        catch (...)
        {
            std::cerr << "[mType LSP] computeImportSpelling failed for "
                      << symbolFilePath << " from " << referencingFilePath
                      << ": unknown exception" << std::endl;
            return symbolFilePath;
        }
    }
}
