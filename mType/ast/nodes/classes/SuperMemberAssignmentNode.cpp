#include "SuperMemberAssignmentNode.hpp"
#include "../../ASTVisitor.hpp"

namespace ast::nodes::classes
{
    SuperMemberAssignmentNode::SuperMemberAssignmentNode(
        const std::string& member,
        std::unique_ptr<ASTNode> val,
        const SourceLocation& loc)
        : ASTNode(loc), memberName(member), value(std::move(val))
    {
    }

    const std::string& SuperMemberAssignmentNode::getMemberName() const
    {
        return memberName;
    }

    void SuperMemberAssignmentNode::setMemberName(const std::string& member)
    {
        memberName = member;
    }

    ASTNode* SuperMemberAssignmentNode::getValue() const
    {
        return value.get();
    }

    void SuperMemberAssignmentNode::setValue(std::unique_ptr<ASTNode> val)
    {
        value = std::move(val);
    }

    Value SuperMemberAssignmentNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitSuperMemberAssignmentNode(this);
    }

    std::unique_ptr<ASTNode> SuperMemberAssignmentNode::clone() const
    {
        return std::make_unique<SuperMemberAssignmentNode>(
            memberName,
            value ? value->clone() : nullptr,
            getLocation());
    }
}
