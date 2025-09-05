#pragma once
#include "../../ASTNode.hpp"
#include <string>

namespace ast::nodes::expressions
{
    class StringNode : public ASTNode
    {
    private:
        std::string value;

    public:
        explicit StringNode(const std::string& val, const SourceLocation& loc = SourceLocation());

        const std::string& getValue() const;
        void setValue(const std::string& val);

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}
