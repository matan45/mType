#include "DoWhileNode.hpp"

namespace ast::nodes::statements
{
    DoWhileNode::DoWhileNode(std::unique_ptr<ASTNode> bodyStmt, std::unique_ptr<ASTNode> cond,
                             const SourceLocation& loc)
        : ASTNode(loc), body(std::move(bodyStmt)), condition(std::move(cond))
    {
    }

    ASTNode* DoWhileNode::getBody() const
    {
        return body.get();
    }

    ASTNode* DoWhileNode::getCondition() const
    {
        return condition.get();
    }

    void DoWhileNode::setBody(std::unique_ptr<ASTNode> bodyStmt)
    {
        body = std::move(bodyStmt);
    }

    void DoWhileNode::setCondition(std::unique_ptr<ASTNode> cond)
    {
        condition = std::move(cond);
    }

    Value DoWhileNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitDoWhileNode(this);
    }
}
