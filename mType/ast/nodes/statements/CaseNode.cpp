#include "CaseNode.hpp"

namespace ast::nodes::statements
{
    CaseNode::CaseNode(std::unique_ptr<ASTNode> caseValue, const SourceLocation& loc)
        : ASTNode(loc), value(std::move(caseValue))
    {
    }

    ASTNode* CaseNode::getValue() const
    {
        return value.get();
    }

    const std::vector<std::unique_ptr<ASTNode>>& CaseNode::getStatements() const
    {
        return statements;
    }

    void CaseNode::setValue(std::unique_ptr<ASTNode> caseValue)
    {
        value = std::move(caseValue);
    }

    void CaseNode::addStatement(std::unique_ptr<ASTNode> statement)
    {
        statements.push_back(std::move(statement));
    }

    size_t CaseNode::getStatementCount() const
    {
        return statements.size();
    }

    Value CaseNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitCaseNode(this);
    }
}
