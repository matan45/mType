#include "WorkspaceSymbolIndex.hpp"

#include "../DocumentManager.hpp"  // full definition of SymbolLocationInfo
#include "../utils/MtFileWalker.hpp"
#include "../utils/UriUtils.hpp"
#include "../utils/MemoryFileReader.hpp"
#include "SymbolRegistrationVisitor.hpp"

#include "../../../mType/lexer/Lexer.hpp"
#include "../../../mType/parser/Parser.hpp"
#include "../../../mType/services/ImportManager.hpp"
#include "../../../mType/services/FileReader.hpp"
#include "../../../mType/environment/EnvironmentBuilder.hpp"
#include "../../../mType/ast/nodes/classes/ClassNode.hpp"
#include "../../../mType/ast/nodes/classes/InterfaceNode.hpp"
#include "../../../mType/ast/nodes/functions/FunctionNode.hpp"
#include "../../../mType/ast/nodes/statements/ProgramNode.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace mtype::lsp::analysis
{
    namespace
    {
        // ImportResolver / SymbolRegistrationVisitor builds qualified
        // names like "Foo.method" for class members. The workspace
        // index only cares about top-level names; filter the rest.
        bool isTopLevelKey(const std::string& key)
        {
            return key.find('.') == std::string::npos;
        }

        std::string toLowerAscii(std::string text)
        {
            std::transform(text.begin(), text.end(), text.begin(),
                [](unsigned char c) {
                    return static_cast<char>(std::tolower(c));
                });
            return text;
        }

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

        // Best-effort kind detection by inspecting top-level program
        // children. The visitor doesn't return kind, so we walk the
        // ProgramNode children directly.
        WorkspaceSymbolKind detectKindFromAst(
            ast::ASTNode* program,
            const std::string& name)
        {
            auto* prog = dynamic_cast<ast::ProgramNode*>(program);
            if (!prog) return WorkspaceSymbolKind::Unknown;
            for (const auto& stmt : prog->getStatements())
            {
                if (auto* cls = dynamic_cast<ast::ClassNode*>(stmt.get()))
                {
                    if (cls->getClassName() == name) return WorkspaceSymbolKind::Class;
                }
                if (auto* iface = dynamic_cast<ast::InterfaceNode*>(stmt.get()))
                {
                    if (iface->getName() == name) return WorkspaceSymbolKind::Interface;
                }
                if (auto* fn = dynamic_cast<ast::FunctionNode*>(stmt.get()))
                {
                    if (fn->getName() == name) return WorkspaceSymbolKind::Function;
                }
            }
            return WorkspaceSymbolKind::Unknown;
        }
    } // namespace

    namespace
    {
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
    }

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

        std::lock_guard<std::mutex> lock(mutex_);
        byName_.clear();
        byFile_.clear();

        for (const auto& path : files)
        {
            std::string content;
            if (!readFileFromDisk(path, content)) continue;
            indexFileLocked(path, content);
        }
    }

    void WorkspaceSymbolIndex::reindexFile(const std::string& fileUri)
    {
        // Disk fallback path. Used when we don't have an in-memory buffer
        // (e.g. a file the editor hasn't opened yet, or a refresh
        // triggered by a file-system event for a closed document).
        const std::string path = UriUtils::uriToFilePath(fileUri);
        std::string content;
        if (!readFileFromDisk(path, content)) return;

        std::lock_guard<std::mutex> lock(mutex_);
        dropEntriesLocked(fileUri);
        indexFileLocked(path, content);
    }

    void WorkspaceSymbolIndex::reindexFile(const std::string& fileUri,
                                           const std::string& content)
    {
        // Buffer path. Preferred when the LSP has the live editor content
        // — without this, the auto-import quick fix would not see symbols
        // declared in unsaved files until they were saved.
        const std::string path = UriUtils::uriToFilePath(fileUri);

        std::lock_guard<std::mutex> lock(mutex_);
        dropEntriesLocked(fileUri);
        indexFileLocked(path, content);
    }

    void WorkspaceSymbolIndex::invalidateFile(const std::string& fileUri)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        dropEntriesLocked(fileUri);
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

    void WorkspaceSymbolIndex::dropEntriesLocked(const std::string& fileUri)
    {
        auto byFileIt = byFile_.find(fileUri);
        if (byFileIt == byFile_.end()) return;

        for (const auto& name : byFileIt->second)
        {
            auto byNameIt = byName_.find(name);
            if (byNameIt == byName_.end()) continue;
            auto& vec = byNameIt->second;
            vec.erase(
                std::remove_if(vec.begin(), vec.end(),
                    [&fileUri](const WorkspaceSymbol& s) {
                        return s.fileUri == fileUri;
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

        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<WorkspaceSymbol> results;

        for (const auto& [name, symbols] : byName_)
        {
            if (!startsWithCaseInsensitive(name, prefix)) continue;
            results.insert(results.end(), symbols.begin(), symbols.end());
        }

        const std::string loweredPrefix = toLowerAscii(prefix);
        std::sort(results.begin(), results.end(),
            [&loweredPrefix](const WorkspaceSymbol& a, const WorkspaceSymbol& b)
            {
                const std::string aLower = toLowerAscii(a.name);
                const std::string bLower = toLowerAscii(b.name);
                const bool aExact = aLower == loweredPrefix;
                const bool bExact = bLower == loweredPrefix;
                if (aExact != bExact) return aExact;
                if (aLower != bLower) return aLower < bLower;
                if (a.name != b.name) return a.name < b.name;
                return a.fileUri < b.fileUri;
            });

        if (results.size() > maxResults)
        {
            results.resize(maxResults);
        }
        return results;
    }

    void WorkspaceSymbolIndex::indexFileLocked(const std::string& filePath,
                                                const std::string& content)
    {
        if (filePath.empty()) return;

        // Caller supplies the content (either from disk in the
        // buildFromWorkspace / disk-fallback path, or from the live
        // editor buffer via the buffer overload of reindexFile). This
        // method just runs the parse pipeline against it.
        try
        {
            const std::string fileUri = UriUtils::filePathToUri(filePath);

            auto memReader = std::make_unique<MemoryFileReader>();
            memReader->setContent(fileUri, content);

            lexer::Lexer lex(fileUri, std::move(memReader));
            auto importMgr = std::make_unique<services::ImportManager>();
            parser::Parser parser(lex, std::move(importMgr));

            auto program = parser.parseProgram();
            if (!program) return;

            auto env = environment::EnvironmentBuilder::createDefault();
            SymbolRegistrationVisitor visitor(env);
            visitor.processProgram(program.get(), fileUri);

            std::vector<std::string> indexedNames;
            for (const auto& [key, info] : visitor.getSymbolLocations())
            {
                if (!isTopLevelKey(key)) continue;

                WorkspaceSymbol sym;
                sym.name = key;
                sym.fileUri = fileUri;
                sym.line = info.line;
                sym.column = info.column;
                sym.kind = detectKindFromAst(program.get(), key);

                byName_[key].push_back(sym);
                indexedNames.push_back(key);
            }

            if (!indexedNames.empty())
            {
                byFile_[fileUri] = std::move(indexedNames);
            }
        }
        catch (const std::exception& e)
        {
            // Log at debug level so workspace indexing failures are
            // diagnosable. We don't propagate — one bad file shouldn't
            // bring down the whole index, and the user will see proper
            // diagnostics for the file when they open it.
            std::cerr << "[mType LSP] Workspace index parse failed for "
                      << filePath << ": " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "[mType LSP] Workspace index parse failed for "
                      << filePath << ": unknown exception" << std::endl;
        }
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
