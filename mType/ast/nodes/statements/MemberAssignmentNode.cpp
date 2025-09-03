#include "MemberAssignmentNode.hpp"

namespace ast::nodes::statements
{
    MemberAssignmentNode::MemberAssignmentNode(std::shared_ptr<ASTNode> obj, const std::string& member,
                                               std::shared_ptr<ASTNode> val, const SourceLocation& loc)
        : ASTNode(loc), object(std::move(obj)), memberName(member), value(std::move(val))
    {
    }

    MemberAssignmentNode::MemberAssignmentNode(std::unique_ptr<ASTNode> obj, const std::string& member,
                                               std::unique_ptr<ASTNode> val, const SourceLocation& loc)
        : ASTNode(loc), object(std::move(obj)), memberName(member), value(std::move(val))
    {
    }

    ASTNode* MemberAssignmentNode::getObject() const noexcept
    {
        return object.get();
    }

    const std::string& MemberAssignmentNode::getMemberName() const
    {
        return memberName;
    }

    ASTNode* MemberAssignmentNode::getValue() const noexcept
    {
        return value.get();
    }

    std::shared_ptr<ASTNode> MemberAssignmentNode::getObjectShared() const
    {
        return object;
    }

    std::shared_ptr<ASTNode> MemberAssignmentNode::getValueShared() const
    {
        return value;
    }

    void MemberAssignmentNode::setObject(std::shared_ptr<ASTNode> obj)
    {
        object = std::move(obj);
    }

    void MemberAssignmentNode::setMemberName(const std::string& member)
    {
        memberName = member;
    }

    void MemberAssignmentNode::setValue(std::shared_ptr<ASTNode> val)
    {
        value = std::move(val);
    }

    Value MemberAssignmentNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitMemberAssignmentNode(this);
    }
}
