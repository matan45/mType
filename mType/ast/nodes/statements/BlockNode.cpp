#include "BlockNode.hpp"
#include "../../utils/ASTNodeUtils.hpp"

namespace ast::nodes::statements
{
    BlockNode::BlockNode(std::vector<std::unique_ptr<ASTNode>> stmts, const SourceLocation& loc)
        : ASTNode(loc), statements(std::move(stmts))
    {
    }

    const std::vector<std::unique_ptr<ASTNode>>& BlockNode::getStatements() const
    {
        return statements;
    }

    void BlockNode::addStatement(std::unique_ptr<ASTNode> statement)
    {
        statements.push_back(std::move(statement));
    }

    void BlockNode::setStatements(std::vector<std::unique_ptr<ASTNode>> stmts)
    {
        statements = std::move(stmts);
    }

    size_t BlockNode::getStatementCount() const
    {
        return statements.size();
    }

    Value BlockNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitBlockNode(this);
    }

    std::unique_ptr<ASTNode> BlockNode::clone() const
    {
        return std::make_unique<BlockNode>(ast::utils::cloneNodeVector(statements), location);
    }
}
