#pragma once
#include "../../ASTNode.hpp"
#include <string>

namespace ast::nodes::expressions
{
    class VariableNode : public ASTNode
    {
    private:
        std::string name;

    public:
        explicit VariableNode(const std::string& varName, const SourceLocation& loc = SourceLocation());

        const std::string& getName() const;
        void setName(const std::string& varName);

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
