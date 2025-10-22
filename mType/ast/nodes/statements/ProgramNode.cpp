#include "ProgramNode.hpp"
#include "../../utils/ASTNodeUtils.hpp"

namespace ast::nodes::statements
{
    ProgramNode::ProgramNode(const SourceLocation& loc)
        : ASTNode(loc)
    {
    }

    ProgramNode::ProgramNode(std::vector<std::unique_ptr<ASTNode>> stmts, const SourceLocation& loc)
        : ASTNode(loc), statements(std::move(stmts))
    {
    }

    const std::vector<std::unique_ptr<ASTNode>>& ProgramNode::getStatements() const
    {
        return statements;
    }

    void ProgramNode::addStatement(std::unique_ptr<ASTNode> statement)
    {
        statements.push_back(std::move(statement));
    }

    void ProgramNode::setStatements(std::vector<std::unique_ptr<ASTNode>> stmts)
    {
        statements = std::move(stmts);
    }

    size_t ProgramNode::getStatementCount() const
    {
        return statements.size();
    }

    Value ProgramNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitProgramNode(this);
    }

    std::unique_ptr<ASTNode> ProgramNode::clone() const
    {
        return std::make_unique<ProgramNode>(ast::utils::cloneNodeVector(statements), location);
    }
}
