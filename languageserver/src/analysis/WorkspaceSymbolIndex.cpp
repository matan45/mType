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
#include <filesystem>
#include <fstream>

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

    void WorkspaceSymbolIndex::buildFromWorkspace(const std::string& workspaceRoot)
    {
        const auto files = utils::MtFileWalker::findMtFiles(workspaceRoot);

        std::lock_guard<std::mutex> lock(mutex_);
        byName_.clear();
        byFile_.clear();

        for (const auto& path : files)
        {
            indexFileLocked(path);
        }
    }

    void WorkspaceSymbolIndex::reindexFile(const std::string& fileUri)
    {
        // Convert URI back to a filesystem path for parsing.
        const std::string path = UriUtils::uriToFilePath(fileUri);

        std::lock_guard<std::mutex> lock(mutex_);
        // Drop existing entries for this URI before re-indexing.
        auto byFileIt = byFile_.find(fileUri);
        if (byFileIt != byFile_.end())
        {
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

        indexFileLocked(path);
    }

    void WorkspaceSymbolIndex::invalidateFile(const std::string& fileUri)
    {
        std::lock_guard<std::mutex> lock(mutex_);
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

    void WorkspaceSymbolIndex::indexFileLocked(const std::string& filePath)
    {
        if (filePath.empty()) return;

        // Re-run the parse pipeline. We don't share state with
        // DocumentManager because the workspace index needs disk
        // contents, not editor buffers — the editor buffer for an
        // open document gets refreshed via reindexFile() which still
        // reads from disk (acceptable: the user just saved).
        try
        {
            std::ifstream stream(filePath);
            if (!stream.is_open()) return;
            std::string content(
                (std::istreambuf_iterator<char>(stream)),
                std::istreambuf_iterator<char>());

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
        catch (...)
        {
            // Silently swallow — this file will get its own diagnostics
            // when the user opens it.
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

            // Drop the .mt extension to match existing import convention.
            if (rel.extension() == ".mt")
            {
                rel.replace_extension();
            }

            // Normalize separators to forward slashes.
            std::string spelling = rel.generic_string();

            // Ensure relative imports start with `./` so the parser
            // recognises them as relative.
            if (!spelling.empty() && spelling[0] != '.' && spelling[0] != '/')
            {
                spelling = "./" + spelling;
            }
            return spelling;
        }
        catch (...)
        {
            return symbolFilePath;
        }
    }
}
