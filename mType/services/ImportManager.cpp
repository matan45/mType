#include "ImportManager.hpp"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include "../lexer/Lexer.hpp"
#include "../parser/Parser.hpp"


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

    std::string ImportManager::resolvePath(const std::string& path)
    {
        fs::path filePath(path);

        // If path is relative, resolve it relative to current directory
        if (filePath.is_relative())
        {
            filePath = fs::path(baseDirectory) / filePath;
        }

        // Normalize the path (resolve . and .. components)
        filePath = fs::canonical(filePath);

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
        try
        {
            filePath = fs::canonical(filePath);
        }
        catch (const std::filesystem::filesystem_error&)
        {
            // If canonical fails, at least get the absolute path
            filePath = fs::absolute(filePath);
        }

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
        try
        {
            std::string resolvedPath = resolvePath(rawPath);
            return importedFiles.find(resolvedPath) != importedFiles.end();
        }
        catch (...)
        {
            return false;
        }
    }

    bool ImportManager::isEvaluated(const std::string& rawPath)
    {
        try
        {
            std::string resolvedPath = resolvePathConsistently(rawPath);
            return evaluatedFiles.find(resolvedPath) != evaluatedFiles.end();
        }
        catch (...)
        {
            return false;
        }
    }

    void ImportManager::markAsEvaluated(const std::string& rawPath)
    {
        try
        {
            std::string resolvedPath = resolvePathConsistently(rawPath);
            evaluatedFiles.insert(resolvedPath);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error marking as evaluated: " << e.what() << std::endl;
            // Ignore errors when marking as evaluated
        }
    }

    bool ImportManager::isBeingEvaluated(const std::string& rawPath)
    {
        try
        {
            std::string resolvedPath = resolvePathConsistently(rawPath);
            return beingEvaluated.find(resolvedPath) != beingEvaluated.end();
        }
        catch (...)
        {
            return false;
        }
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
            std::cerr << "Error marking as being evaluated: " << e.what() << std::endl;
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
            std::cerr << "Error unmarking as being evaluated: " << e.what() << std::endl;
        }
    }

    std::vector<std::string> ImportManager::getImportedFiles() const
    {
        return std::vector<std::string>(importedFiles.begin(), importedFiles.end());
    }
}
