#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <memory>

namespace ast::nodes::classes
{
    class MemberAccessNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> object;
        std::string memberName;
        bool isStaticAccess; // For Class::member vs obj.member

    public:
        explicit MemberAccessNode(std::unique_ptr<ASTNode> obj, const std::string& member, 
                         bool isStatic = false, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), object(std::move(obj)), memberName(member), isStaticAccess(isStatic) {}

        ASTNode* getObject() const { return object.get(); }
        const std::string& getMemberName() const { return memberName; }
        bool getIsStaticAccess() const { return isStaticAccess; }

        void setObject(std::unique_ptr<ASTNode> obj) { object = std::move(obj); }
        void setMemberName(const std::string& member) { memberName = member; }
        void setIsStaticAccess(bool isStatic) { isStaticAccess = isStatic; }
        
        // Release ownership of the object node
        std::unique_ptr<ASTNode> releaseObject() { return std::move(object); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitMemberAccessNode(this);
        }
    };
}
