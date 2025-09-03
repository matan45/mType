#include "DefaultCaseNode.hpp"

namespace ast::nodes::statements
{
    DefaultCaseNode::DefaultCaseNode(const SourceLocation& loc)
        : ASTNode(loc)
    {
    }

    const std::vector<std::unique_ptr<ASTNode>>& DefaultCaseNode::getStatements() const
    {
        return statements;
    }

    void DefaultCaseNode::addStatement(std::unique_ptr<ASTNode> statement)
    {
        statements.push_back(std::move(statement));
    }

    size_t DefaultCaseNode::getStatementCount() const
    {
        return statements.size();
    }

    Value DefaultCaseNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitDefaultCaseNode(this);
    }
}
