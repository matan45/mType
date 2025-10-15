#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <memory>
#include <vector>
#include <filesystem>

namespace ast::nodes::statements
{
    /**
     * @brief Import type for the new import system
     */
    enum class ImportType
    {
        SELECTIVE,  // import {A, B, C} from "file.mt"
        WILDCARD    // import * from "file.mt"
    };

    class ImportNode : public ASTNode
    {
    private:
        std::string filePath;
        ImportType importType;
        std::vector<std::string> importedSymbols;  // For selective imports
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