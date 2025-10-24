#pragma once
#include "../../ASTNode.hpp"
#include <string>
#include <memory>

namespace ast::nodes::classes
{
    class SuperMemberAssignmentNode : public ASTNode
    {
    private:
        std::string memberName;
        std::unique_ptr<ASTNode> value;

    public:
        explicit SuperMemberAssignmentNode(
            const std::string& member,
            std::unique_ptr<ASTNode> val,
            const SourceLocation& loc = SourceLocation());

        const std::string& getMemberName() const;
        void setMemberName(const std::string& member);

        ASTNode* getValue() const;
        void setValue(std::unique_ptr<ASTNode> val);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
