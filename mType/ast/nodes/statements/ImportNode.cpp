#include "ImportNode.hpp"
#include <filesystem>

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

    std::string ImportNode::getResolvedImportPath(const std::string& baseDirectory) const
    {
        std::filesystem::path path(filePath);

        // If path is relative, resolve it relative to base directory
        if (path.is_relative() && !baseDirectory.empty()) {
            path = std::filesystem::path(baseDirectory) / path;
        }

        // Check if .mtc version exists and prefer it
        std::string mtcPath = convertToMtcPath(path.string());
        if (std::filesystem::exists(mtcPath)) {
            return mtcPath;
        }

        // Fall back to original .mt path
        return path.string();
    }

    std::string ImportNode::convertToMtcPath(const std::string& mtPath) const
    {
        std::filesystem::path path(mtPath);

        // If already .mtc, return as-is
        if (path.extension() == ".mtc") {
            return mtPath;
        }

        // Convert .mt to .mtc
        path.replace_extension(".mtc");
        return path.string();
    }

    bool ImportNode::prefersMtcFile() const
    {
        // Check if a .mtc version exists for this import
        std::string mtcPath = convertToMtcPath(filePath);
        return std::filesystem::exists(mtcPath);
    }

    Value ImportNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitImportNode(this);
    }
}
