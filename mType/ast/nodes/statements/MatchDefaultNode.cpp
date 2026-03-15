#include "MatchDefaultNode.hpp"
#include "../../utils/ASTNodeUtils.hpp"

namespace ast::nodes::statements
{
    MatchDefaultNode::MatchDefaultNode(std::unique_ptr<ASTNode> body, const SourceLocation& loc)
        : ASTNode(loc), body(std::move(body))
    {
    }

    ASTNode* MatchDefaultNode::getBody() const { return body.get(); }

    void MatchDefaultNode::setBody(std::unique_ptr<ASTNode> newBody)
    {
        body = std::move(newBody);
    }

    Value MatchDefaultNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitMatchDefaultNode(this);
    }

    std::unique_ptr<ASTNode> MatchDefaultNode::clone() const
    {
        return std::make_unique<MatchDefaultNode>(
            ast::utils::cloneNodeSafe(body), location);
    }
}
