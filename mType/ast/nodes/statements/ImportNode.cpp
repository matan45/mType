#include "ImportNode.hpp"
#include <filesystem>

namespace ast::nodes::statements
{
    // New constructor for selective imports
    ImportNode::ImportNode(const std::string& path,
                          ImportType type,
                          const std::vector<std::string>& symbols,
                          const SourceLocation& loc)
        : ASTNode(loc), filePath(path), importType(type), importedSymbols(symbols), importedAST(nullptr)
    {
    }

    // New constructor for wildcard imports
    ImportNode::ImportNode(const std::string& path,
                          ImportType type,
                          const SourceLocation& loc)
        : ASTNode(loc), filePath(path), importType(type), importedAST(nullptr)
    {
    }

    // Backward compatibility constructor (for AST caching)
    ImportNode::ImportNode(const std::string& path, ASTNode* ast, const SourceLocation& loc)
        : ASTNode(loc), filePath(path), importType(ImportType::WILDCARD), importedAST(ast)
    {
    }

    const std::string& ImportNode::getFilePath() const
    {
        return filePath;
    }

    ImportType ImportNode::getImportType() const
    {
        return importType;
    }

    const std::vector<std::string>& ImportNode::getImportedSymbols() const
    {
        return importedSymbols;
    }

    bool ImportNode::isWildcard() const
    {
        return importType == ImportType::WILDCARD;
    }

    bool ImportNode::isSelective() const
    {
        return importType == ImportType::SELECTIVE;
    }

    ASTNode* ImportNode::getImportedAST() const
    {
        return importedAST;
    }

    const std::vector<std::unique_ptr<ASTNode>>& ImportNode::getImportedDeclarations() const
    {
        return importedDeclarations;
    }

    void ImportNode::setFilePath(const std::string& path)
    {
        filePath = path;
    }

    void ImportNode::setImportType(ImportType type)
    {
        importType = type;
    }

    void ImportNode::setImportedSymbols(const std::vector<std::string>& symbols)
    {
        importedSymbols = symbols;
    }

    void ImportNode::addImportedSymbol(const std::string& symbol)
    {
        importedSymbols.push_back(symbol);
    }

    void ImportNode::setImportedAST(ASTNode* ast)
    {
        importedAST = ast;
    }

    void ImportNode::addImportedDeclaration(std::unique_ptr<ASTNode> decl)
    {
        importedDeclarations.push_back(std::move(decl));
    }

    bool ImportNode::isResolved() const
    {
        return importedAST != nullptr;
    }

    Value ImportNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitImportNode(this);
    }

    std::unique_ptr<ASTNode> ImportNode::clone() const
    {
        // Clone imported declarations
        std::vector<std::unique_ptr<ASTNode>> clonedDeclarations;
        clonedDeclarations.reserve(importedDeclarations.size());
        for (const auto& decl : importedDeclarations) {
            if (decl) {
                clonedDeclarations.push_back(decl->clone());
            }
        }

        // Clone the import node
        auto clonedImport = std::make_unique<ImportNode>(filePath, importType, importedSymbols, location);

        // Set the imported AST (non-owning pointer, just copy)
        clonedImport->setImportedAST(importedAST);

        // Move cloned declarations
        for (auto& decl : clonedDeclarations) {
            clonedImport->addImportedDeclaration(std::move(decl));
        }

        return clonedImport;
    }
}
