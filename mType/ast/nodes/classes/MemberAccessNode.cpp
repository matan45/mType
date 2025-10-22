#include "MemberAccessNode.hpp"

namespace ast::nodes::classes
{
    MemberAccessNode::MemberAccessNode(std::shared_ptr<ASTNode> obj, const std::string& member, bool isStatic,
                                       const SourceLocation& loc)
        : ASTNode(loc), object(std::move(obj)), memberName(member), isStaticAccess(isStatic)
    {
    }

    MemberAccessNode::MemberAccessNode(std::unique_ptr<ASTNode> obj, const std::string& member, bool isStatic,
                                       const SourceLocation& loc)
        : ASTNode(loc), object(std::move(obj)), memberName(member), isStaticAccess(isStatic)
    {
    }

    ASTNode* MemberAccessNode::getObject() const noexcept
    {
        return object.get();
    }

    const std::string& MemberAccessNode::getMemberName() const
    {
        return memberName;
    }

    bool MemberAccessNode::getIsStaticAccess() const
    {
        return isStaticAccess;
    }

    std::shared_ptr<ASTNode> MemberAccessNode::getObjectShared() const
    {
        return object;
    }

    void MemberAccessNode::setObject(std::shared_ptr<ASTNode> obj)
    {
        object = std::move(obj);
    }

    void MemberAccessNode::setMemberName(const std::string& member)
    {
        memberName = member;
    }

    void MemberAccessNode::setIsStaticAccess(bool isStatic)
    {
        isStaticAccess = isStatic;
    }

    Value MemberAccessNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitMemberAccessNode(this);
    }

    std::unique_ptr<ASTNode> MemberAccessNode::clone() const
    {
        // Clone the object (shared_ptr -> unique_ptr -> shared_ptr)
        std::shared_ptr<ASTNode> clonedObject = object ? std::shared_ptr<ASTNode>(object->clone()) : nullptr;

        return std::make_unique<MemberAccessNode>(
            clonedObject,
            memberName,
            isStaticAccess,
            location
        );
    }
}
