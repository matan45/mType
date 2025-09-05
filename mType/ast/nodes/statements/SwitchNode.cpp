#include "SwitchNode.hpp"

namespace ast::nodes::statements
{
    SwitchNode::SwitchNode(std::unique_ptr<ASTNode> expr, const SourceLocation& loc)
        : ASTNode(loc), expression(std::move(expr))
    {
    }

    ASTNode* SwitchNode::getExpression() const
    {
        return expression.get();
    }

    const std::vector<std::unique_ptr<ASTNode>>& SwitchNode::getCases() const
    {
        return cases;
    }

    void SwitchNode::setExpression(std::unique_ptr<ASTNode> expr)
    {
        expression = std::move(expr);
    }

    void SwitchNode::addCase(std::unique_ptr<ASTNode> caseNode)
    {
        cases.push_back(std::move(caseNode));
    }

    size_t SwitchNode::getCaseCount() const
    {
        return cases.size();
    }

    Value SwitchNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitSwitchNode(this);
    }
}
