#include "SuperMemberAccessNode.hpp"
#include "../../ASTVisitor.hpp"

namespace ast::nodes::classes
{
    SuperMemberAccessNode::SuperMemberAccessNode(
        const std::string& member,
        const SourceLocation& loc)
        : ASTNode(loc), memberName(member)
    {
    }

    const std::string& SuperMemberAccessNode::getMemberName() const
    {
        return memberName;
    }

    void SuperMemberAccessNode::setMemberName(const std::string& member)
    {
        memberName = member;
    }

    Value SuperMemberAccessNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitSuperMemberAccessNode(this);
    }

    std::unique_ptr<ASTNode> SuperMemberAccessNode::clone() const
    {
        return std::make_unique<SuperMemberAccessNode>(memberName, getLocation());
    }
}
