#include "MatchNode.hpp"
#include <cstddef>
#include "../../utils/ASTNodeUtils.hpp"

namespace ast::nodes::statements
{
    MatchNode::MatchNode(std::unique_ptr<ASTNode> expr, const SourceLocation& loc)
        : ASTNode(loc), expression(std::move(expr))
    {
    }

    ASTNode* MatchNode::getExpression() const { return expression.get(); }

    const std::vector<std::unique_ptr<ASTNode>>& MatchNode::getCases() const
    {
        return cases;
    }

    void MatchNode::setExpression(std::unique_ptr<ASTNode> expr)
    {
        expression = std::move(expr);
    }

    void MatchNode::addCase(std::unique_ptr<ASTNode> caseNode)
    {
        cases.push_back(std::move(caseNode));
    }

    size_t MatchNode::getCaseCount() const { return cases.size(); }

    Value MatchNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitMatchNode(this);
    }

    std::unique_ptr<ASTNode> MatchNode::clone() const
    {
        auto clonedExpr = ast::utils::cloneNodeSafe(expression);
        auto clonedMatch = std::make_unique<MatchNode>(std::move(clonedExpr), location);

        for (auto& caseNode : ast::utils::cloneNodeVector(cases)) {
            clonedMatch->addCase(std::move(caseNode));
        }

        return clonedMatch;
    }
}
