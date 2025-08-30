#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <memory>
#include <vector>

namespace ast::nodes::statements
{
    class ImportNode : public ASTNode
    {
    private:
        std::string filePath;
        ASTNode* importedAST; // Reference to cached AST in ImportManager
        std::vector<std::unique_ptr<ASTNode>> importedDeclarations; // Extracted declarations
        void* importManager; // Reference to ImportManager (as void* to avoid circular dependency)

    public:
        explicit ImportNode(const std::string& path, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), filePath(path), importedAST(nullptr), importManager(nullptr) {}

        explicit ImportNode(const std::string& path, ASTNode* ast,
                          const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), filePath(path), importedAST(ast), importManager(nullptr) {}

        const std::string& getFilePath() const { return filePath; }
        ASTNode* getImportedAST() const { return importedAST; }
        const std::vector<std::unique_ptr<ASTNode>>& getImportedDeclarations() const { return importedDeclarations; }

        void setFilePath(const std::string& path) { filePath = path; }
        void setImportedAST(ASTNode* ast) { importedAST = ast; }
        void addImportedDeclaration(std::unique_ptr<ASTNode> decl) { 
            importedDeclarations.push_back(std::move(decl)); 
        }

        // Check if this import was successfully resolved
        bool isResolved() const { return importedAST != nullptr; }
        
        // Get/Set ImportManager reference
        void* getImportManager() const { return importManager; }
        void setImportManager(void* manager) { importManager = manager; }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitImportNode(this);
        }
    };
}