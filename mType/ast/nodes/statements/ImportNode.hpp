#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace ast::nodes::statements
{
    /**
     * @brief Import type for the new import system
     */
    enum class ImportType
    {
        SELECTIVE,           // import {A, B, C} from "file.mt"
        WILDCARD,            // import * from "file.mt"
        LIBRARY,             // import lib "collections"
        LIBRARY_SELECTIVE    // import lib {List, HashMap} from "collections"
    };

    class ImportNode : public ASTNode
    {
    private:
        std::string filePath;
        ImportType importType;
        std::vector<std::string> importedSymbols;  // For selective imports
        std::unordered_map<std::string, std::string> symbolAliases;  // original -> alias (for "X as Y")
        std::string libraryName;  // For library imports (LIBRARY, LIBRARY_SELECTIVE)
        ASTNode* importedAST; // Non-owning reference to cached AST in ImportManager
        std::vector<std::unique_ptr<ASTNode>> importedDeclarations; // Extracted declarations

    public:
        // New constructors for selective/wildcard imports
        explicit ImportNode(const std::string& path,
                          ImportType type,
                          const std::vector<std::string>& symbols,
                          const SourceLocation& loc = SourceLocation());

        explicit ImportNode(const std::string& path,
                          ImportType type,
                          const SourceLocation& loc = SourceLocation());

        explicit ImportNode(const std::string& path, ASTNode* ast,
                          const SourceLocation& loc = SourceLocation());

        const std::string& getFilePath() const;
        ImportType getImportType() const;
        const std::vector<std::string>& getImportedSymbols() const;
        bool isWildcard() const;
        bool isSelective() const;
        bool isLibraryImport() const;
        bool isLibrarySelective() const;

        const std::string& getLibraryName() const;
        void setLibraryName(const std::string& name);

        const std::unordered_map<std::string, std::string>& getSymbolAliases() const;
        void addSymbolAlias(const std::string& original, const std::string& alias);

        ASTNode* getImportedAST() const;
        const std::vector<std::unique_ptr<ASTNode>>& getImportedDeclarations() const;

        void setFilePath(const std::string& path);
        void setImportType(ImportType type);
        void setImportedSymbols(const std::vector<std::string>& symbols);
        void addImportedSymbol(const std::string& symbol);
        void setImportedAST(ASTNode* ast);
        void addImportedDeclaration(std::unique_ptr<ASTNode> decl);

        // Check if this import was successfully resolved
        bool isResolved() const;

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}