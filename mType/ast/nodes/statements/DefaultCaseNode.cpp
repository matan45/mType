#include "DefaultCaseNode.hpp"
#include <cstddef>
#include "../../utils/ASTNodeUtils.hpp"

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

    std::unique_ptr<ASTNode> DefaultCaseNode::clone() const
    {
        auto clonedDefault = std::make_unique<DefaultCaseNode>(location);

        for (auto& stmt : ast::utils::cloneNodeVector(statements)) {
            clonedDefault->addStatement(std::move(stmt));
        }

        return clonedDefault;
    }
}
