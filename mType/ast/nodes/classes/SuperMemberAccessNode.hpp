#pragma once
#include "../../ASTNode.hpp"
#include <string>

namespace ast::nodes::classes
{
    class SuperMemberAccessNode : public ASTNode
    {
    private:
        std::string memberName;

    public:
        explicit SuperMemberAccessNode(
            const std::string& member,
            const SourceLocation& loc = SourceLocation());

        const std::string& getMemberName() const;
        void setMemberName(const std::string& member);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
