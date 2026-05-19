#include "ImportManager.hpp"
#include "../lexer/Lexer.hpp"
#include "../parser/Parser.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"

namespace services
{
    using namespace ast;
    using namespace lexer;
    using namespace parser;

    ImportManager::ImportManager() : baseDirectory(".")
    {
    }

    ImportManager::~ImportManager()
    {
        clearCache();
    }

    ASTNode* ImportManager::parseAndCacheAST(const std::string& rawPath)
    {
        std::string resolvedPath = resolvePath(rawPath);

        auto it = astCache.find(resolvedPath);
        if (it != astCache.end())
        {
            return it->second.get();
        }

        Lexer lexer(resolvedPath);
        // Pass nullptr for ImportManager since we don't handle nested imports
        // during parsing — they're handled during the evaluation phase below.
        Parser parser(lexer, nullptr);
        std::unique_ptr<ASTNode> ast = parser.parseProgram();

        ASTNode* astPtr = ast.get();
        astCache[resolvedPath] = std::move(ast);
        importedFiles.insert(resolvedPath);

        return astPtr;
    }

    void ImportManager::clearCache()
    {
        astCache.clear();
        importedFiles.clear();
        evaluatedFiles.clear();
        beingEvaluated.clear();
    }

    bool ImportManager::isImported(const std::string& rawPath)
    {
        return safeCheckInSet(rawPath, importedFiles, false);
    }

    bool ImportManager::isEvaluated(const std::string& rawPath)
    {
        return safeCheckInSet(rawPath, evaluatedFiles, true);
    }

    void ImportManager::markAsEvaluated(const std::string& rawPath)
    {
        try
        {
            std::string resolvedPath = resolvePathConsistently(rawPath);
            evaluatedFiles.insert(resolvedPath);
        }
        catch (const std::exception&)
        {
            // Ignore errors when marking as evaluated.
        }
    }

    bool ImportManager::isBeingEvaluated(const std::string& rawPath)
    {
        return safeCheckInSet(rawPath, beingEvaluated, true);
    }

    void ImportManager::markAsBeingEvaluated(const std::string& rawPath)
    {
        try
        {
            std::string resolvedPath = resolvePathConsistently(rawPath);
            beingEvaluated.insert(resolvedPath);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(
                "Failed to mark file as being evaluated: " + rawPath +
                ". Error: " + e.what());
        }
    }

    void ImportManager::unmarkAsBeingEvaluated(const std::string& rawPath)
    {
        try
        {
            std::string resolvedPath = resolvePathConsistently(rawPath);
            beingEvaluated.erase(resolvedPath);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(
                "Failed to unmark file as being evaluated: " + rawPath +
                ". Error: " + e.what());
        }
    }

    std::vector<std::string> ImportManager::getImportedFiles() const
    {
        return std::vector<std::string>(importedFiles.begin(), importedFiles.end());
    }

    void ImportManager::resolveAllImports(ast::ASTNode* node)
    {
        if (!node) return;

        if (auto importNode = dynamic_cast<ast::ImportNode*>(node))
        {
            // Library imports are resolved by LibraryLinker, not ImportManager.
            if (importNode->isLibraryImport()) {
                markLibraryLoaded(importNode->getLibraryName());
                return;
            }

            std::string filePath = importNode->getFilePath();

            if (importNode->isResolved() && importNode->getImportedAST())
            {
                handleResolvedImport(importNode);
                return;
            }

            handleUnresolvedImport(importNode, filePath);
            return;
        }

        resolveImportsInChildren(node);
    }

    void ImportManager::handleResolvedImport(ast::ImportNode* importNode)
    {
        std::string savedCurrentFile = currentFilePath;

        // Find the cached resolved path of the imported file so nested
        // imports inside it resolve relative to its own directory.
        for (const auto& [path, ast] : astCache)
        {
            if (ast.get() == importNode->getImportedAST())
            {
                currentFilePath = path;
                break;
            }
        }

        resolveAllImports(importNode->getImportedAST());
        currentFilePath = savedCurrentFile;
    }

    void ImportManager::handleUnresolvedImport(ast::ImportNode* importNode, const std::string& filePath)
    {
        std::string savedCurrentFile = currentFilePath;

        std::string resolvedPath = resolvePath(filePath);
        markAsBeingEvaluated(resolvedPath);

        try
        {
            // Set current file to the resolved path BEFORE parsing so any
            // imports inside the imported file resolve relative to its own
            // directory.
            currentFilePath = resolvedPath;

            // IMPORTANT: pass resolvedPath, not filePath, to avoid double
            // resolution inside parseAndCacheAST.
            ASTNode* importedAST = parseAndCacheAST(resolvedPath);
            importNode->setImportedAST(importedAST);

            resolveAllImports(importedAST);

            markAsEvaluated(resolvedPath);

            currentFilePath = savedCurrentFile;
        }
        catch (...)
        {
            currentFilePath = savedCurrentFile;
            unmarkAsBeingEvaluated(resolvedPath);
            throw;
        }

        unmarkAsBeingEvaluated(resolvedPath);
    }

    void ImportManager::resolveImportsInChildren(ast::ASTNode* node)
    {
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                resolveAllImports(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                resolveAllImports(statement.get());
            }
        }
    }

    bool ImportManager::isLibraryLoaded(const std::string& libraryName) const
    {
        return loadedLibraries.count(libraryName) > 0;
    }

    void ImportManager::markLibraryLoaded(const std::string& libraryName)
    {
        loadedLibraries.insert(libraryName);
    }
}
