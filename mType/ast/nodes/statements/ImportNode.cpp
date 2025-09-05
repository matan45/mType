#include "ImportNode.hpp"

namespace ast::nodes::statements
{
    // Implementation is in the header file (inline)
    ImportNode::ImportNode(const std::string& path, const SourceLocation& loc)
        : ASTNode(loc), filePath(path), importedAST(nullptr)
    {
    }

    ImportNode::ImportNode(const std::string& path, ASTNode* ast, const SourceLocation& loc)
        : ASTNode(loc), filePath(path), importedAST(ast)
    {
    }

    const std::string& ImportNode::getFilePath() const
    {
        return filePath;
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
}
