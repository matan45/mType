#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <memory>

namespace ast::nodes::statements
{
    class MemberAssignmentNode : public ASTNode
    {
    private:
        std::unique_ptr<ASTNode> object;
        std::string memberName;
        std::unique_ptr<ASTNode> value;

    public:
        explicit MemberAssignmentNode(std::unique_ptr<ASTNode> obj, const std::string& member, std::unique_ptr<ASTNode> val,
                             const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), object(std::move(obj)), memberName(member), value(std::move(val)) {}

        std::unique_ptr<ASTNode> getObject() const { return object.get(); }
        const std::string& getMemberName() const { return memberName; }
        std::unique_ptr<ASTNode> getValue() const { return value.get(); }

        void setObject(std::unique_ptr<ASTNode> obj) { object = std::move(obj); }
        void setMemberName(const std::string& member) { memberName = member; }
        void setValue(std::unique_ptr<ASTNode> val) { value = std::move(val); }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitMemberAssignmentNode(this);
        }
    };
}
