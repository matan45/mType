#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <memory>
#include <vector>
#include <filesystem>

namespace ast::nodes::statements
{
    class ImportNode : public ASTNode
    {
    private:
        std::string filePath;
        ASTNode* importedAST; // Non-owning reference to cached AST in ImportManager
        std::vector<std::unique_ptr<ASTNode>> importedDeclarations; // Extracted declarations

    public:
        explicit ImportNode(const std::string& path, const SourceLocation& loc = SourceLocation());

        explicit ImportNode(const std::string& path, ASTNode* ast,
                          const SourceLocation& loc = SourceLocation());

        const std::string& getFilePath() const;
        ASTNode* getImportedAST() const;
        const std::vector<std::unique_ptr<ASTNode>>& getImportedDeclarations() const;

        void setFilePath(const std::string& path);
        void setImportedAST(ASTNode* ast);
        void addImportedDeclaration(std::unique_ptr<ASTNode> decl);

        // Check if this import was successfully resolved
        bool isResolved() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}