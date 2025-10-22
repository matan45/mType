#include "ImportManager.hpp"
#include <filesystem>
#include <algorithm>
#include "../lexer/Lexer.hpp"
#include "../parser/Parser.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"


namespace services
{
    namespace fs = std::filesystem;
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

    void ImportManager::setBaseDirectory(const std::string& dir)
    {
        baseDirectory = dir;
    }

    std::string ImportManager::getCurrentFilePath() const
    {
        return currentFilePath;
    }

    void ImportManager::setCurrentFilePath(const std::string& path)
    {
        currentFilePath = path;
    }

    std::string ImportManager::resolvePath(const std::string& path)
    {
        fs::path filePath(path);

        // If path is relative, resolve it relative to current file's directory
        // (or base directory if no current file)
        if (filePath.is_relative())
        {
            if (!currentFilePath.empty())
            {
                // Resolve relative to the current file's directory
                fs::path currentFileDir = fs::path(currentFilePath).parent_path();
                filePath = currentFileDir / filePath;
            }
            else
            {
                // No current file - resolve relative to base directory
                filePath = fs::path(baseDirectory) / filePath;
            }
        }

        // Normalize the path (resolve . and .. components)
        filePath = normalizeFilePath(filePath, false);

        return filePath.string();
    }

    ASTNode* ImportManager::parseAndCacheAST(const std::string& rawPath)
    {
        std::string resolvedPath = resolvePath(rawPath);

        // Check if already cached
        auto it = astCache.find(resolvedPath);
        if (it != astCache.end())
        {
            return it->second.get();
        }

        // Parse .mt file directly
        Lexer lexer(resolvedPath);
        Parser parser(lexer, nullptr);
        // Pass nullptr for ImportManager since we don't handle nested imports during parsing

        // Pure parsing only - no ImportManager dependency
        // If the nested file has imports, they will be handled during evaluation phase
        std::unique_ptr<ASTNode> ast = parser.parseProgram();

        // Cache the AST
        ASTNode* astPtr = ast.get();
        astCache[resolvedPath] = std::move(ast);
        importedFiles.insert(resolvedPath);

        return astPtr;
    }

    std::string ImportManager::resolvePathConsistently(const std::string& path)
    {
        fs::path filePath(path);

        // If path is relative, resolve it relative to the ORIGINAL base directory (not current directory)
        if (filePath.is_relative())
        {
            filePath = fs::path(baseDirectory) / filePath;
        }

        // Normalize the path (resolve . and .. components)
        filePath = normalizeFilePath(filePath, true);

        return filePath.string();
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
            // Ignore errors when marking as evaluated
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

        // Handle ImportNode
        if (auto importNode = dynamic_cast<ast::ImportNode*>(node))
        {
            std::string filePath = importNode->getFilePath();

            // If already resolved, just recurse into imported AST
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                handleResolvedImport(importNode);
                return;
            }

            // Handle unresolved import
            handleUnresolvedImport(importNode, filePath);
            return;
        }

        // Recursively process children based on node type
        resolveImportsInChildren(node);
    }

    void ImportManager::handleResolvedImport(ast::ImportNode* importNode)
    {
        // Save current file path and restore after recursion
        std::string savedCurrentFile = currentFilePath;

        // Need to know the resolved path of the imported file to set as current
        // Use the cached resolved path
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
        // Save current file path
        std::string savedCurrentFile = currentFilePath;

        // Resolve the import path (relative to current file)
        std::string resolvedPath = resolvePath(filePath);
        markAsBeingEvaluated(resolvedPath);

        try
        {
            // Set current file to the resolved path BEFORE parsing
            // This ensures that if the imported file has its own imports,
            // they will be resolved relative to the imported file's directory
            currentFilePath = resolvedPath;

            // Parse and cache the imported file
            ASTNode* importedAST = parseAndCacheAST(filePath);
            importNode->setImportedAST(importedAST);

            // Recursively resolve imports in the imported file
            resolveAllImports(importedAST);

            markAsEvaluated(resolvedPath);

            // Restore previous current file
            currentFilePath = savedCurrentFile;
        }
        catch (...)
        {
            // Restore current file on error
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

    bool ImportManager::safeCheckInSet(const std::string& rawPath,
                                       const std::unordered_set<std::string>& set,
                                       bool useConsistentResolve)
    {
        try
        {
            std::string resolvedPath = useConsistentResolve ?
                resolvePathConsistently(rawPath) : resolvePath(rawPath);
            return set.find(resolvedPath) != set.end();
        }
        catch (...)
        {
            return false;
        }
    }

    std::filesystem::path ImportManager::normalizeFilePath(const std::filesystem::path& filePath,
                                                          bool allowFallback)
    {
        if (allowFallback)
        {
            try
            {
                return fs::canonical(filePath);
            }
            catch (const std::filesystem::filesystem_error&)
            {
                // If canonical fails, at least get the absolute path
                return fs::absolute(filePath);
            }
        }
        else
        {
            return fs::canonical(filePath);
        }
    }
}
