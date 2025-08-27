#include "ImportManager.hpp"
#include "../lexer/Lexer.hpp"
#include "../parser/Parser.hpp"
#include "../errors/ImportException.hpp"
#include "../errors/FileException.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace services
{
    namespace fs = std::filesystem;
    using namespace ast;
    using namespace lexer;
    using namespace parser;
    
    ImportManager::ImportManager() : currentDirectory(".")
    {
    }
    
    ImportManager::~ImportManager()
    {
        clearCache();
    }
    
    void ImportManager::setBaseDirectory(const std::string& dir)
    {
        currentDirectory = dir;
    }
    
    std::string ImportManager::resolvePath(const std::string& path)
    {
        fs::path filePath(path);
        
        // If path is relative, resolve it relative to current directory
        if (filePath.is_relative()) {
            filePath = fs::path(currentDirectory) / filePath;
        }
        
        // Normalize the path (resolve . and .. components)
        filePath = fs::canonical(filePath);
        
        return filePath.string();
    }
    
    bool ImportManager::wouldCauseCircularDependency(const std::string& filePath)
    {
        std::string resolvedPath = resolvePath(filePath);
        
        // Check if this file is already in the import stack
        std::stack<std::string> tempStack = importStack;
        while (!tempStack.empty()) {
            if (tempStack.top() == resolvedPath) {
                return true;
            }
            tempStack.pop();
        }
        
        return false;
    }
    
    std::string ImportManager::getCircularDependencyChain(const std::string& filePath)
    {
        std::string resolvedPath = resolvePath(filePath);
        std::vector<std::string> chain;
        
        std::stack<std::string> tempStack = importStack;
        bool foundCycle = false;
        
        while (!tempStack.empty()) {
            std::string current = tempStack.top();
            tempStack.pop();
            chain.push_back(current);
            
            if (current == resolvedPath) {
                foundCycle = true;
                break;
            }
        }
        
        if (!foundCycle) {
            return "";
        }
        
        // Reverse to get proper order
        std::reverse(chain.begin(), chain.end());
        
        // Build the dependency chain string
        std::stringstream ss;
        for (size_t i = 0; i < chain.size(); ++i) {
            ss << chain[i];
            if (i < chain.size() - 1) {
                ss << " -> ";
            }
        }
        ss << " -> " << resolvedPath;
        
        return ss.str();
    }
    
    ASTNode* ImportManager::importFile(const std::string& rawPath)
    {
        std::string resolvedPath;
        
        try {
            resolvedPath = resolvePath(rawPath);
        } catch (const std::filesystem::filesystem_error&) {
            throw errors::FileException("Cannot resolve import path: " + rawPath);
        }
        
        // Check if already imported (return cached AST)
        auto cacheIt = astCache.find(resolvedPath);
        if (cacheIt != astCache.end()) {
            return cacheIt->second.get();
        }
        
        // Check for circular dependency
        if (wouldCauseCircularDependency(resolvedPath)) {
            std::string chain = getCircularDependencyChain(resolvedPath);
            throw errors::ImportException("Circular dependency detected: " + chain);
        }
        
        // Read the file
        std::ifstream file(resolvedPath);
        if (!file.is_open()) {
            throw errors::FileException("Cannot open file: " + resolvedPath);
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string fileContent = buffer.str();
        file.close();
        
        // Push current file onto import stack
        importStack.push(resolvedPath);
        
        try {
            // Save current directory and switch to imported file's directory
            std::string savedDir = currentDirectory;
            fs::path importedFilePath(resolvedPath);
            currentDirectory = importedFilePath.parent_path().string();
            
            // Parse the imported file
            Lexer lexer(fileContent, resolvedPath);
            Parser parser(lexer);
            
            // Set the ImportManager in the parser so nested imports work
            parser.setImportManager(this);
            
            std::unique_ptr<ASTNode> ast = parser.parseProgram();
            
            // Restore previous directory
            currentDirectory = savedDir;
            
            // Pop from import stack
            importStack.pop();
            
            // Cache the AST
            astCache[resolvedPath] = std::move(ast);
            importedFiles.insert(resolvedPath);
            
            return astCache[resolvedPath].get();
        }
        catch (const std::exception& e) {
            // Make sure to pop the stack and restore directory even on error
            importStack.pop();
            throw errors::ImportException("Error parsing imported file " + resolvedPath + ": " + e.what());
        }
    }
    
    void ImportManager::clearCache()
    {
        astCache.clear();
        importedFiles.clear();
        while (!importStack.empty()) {
            importStack.pop();
        }
    }
    
    bool ImportManager::isImported(const std::string& rawPath)
    {
        try {
            std::string resolvedPath = resolvePath(rawPath);
            return importedFiles.find(resolvedPath) != importedFiles.end();
        } catch (...) {
            return false;
        }
    }
    
    std::vector<std::string> ImportManager::getImportedFiles() const
    {
        return std::vector<std::string>(importedFiles.begin(), importedFiles.end());
    }
}