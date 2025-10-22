#include "CaseNode.hpp"
#include "../../utils/ASTNodeUtils.hpp"

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

    std::unique_ptr<ASTNode> CaseNode::clone() const
    {
        auto clonedValue = ast::utils::cloneNodeSafe(value);
        auto clonedCase = std::make_unique<CaseNode>(std::move(clonedValue), location);

        for (auto& stmt : ast::utils::cloneNodeVector(statements)) {
            clonedCase->addStatement(std::move(stmt));
        }

        return clonedCase;
    }
}
