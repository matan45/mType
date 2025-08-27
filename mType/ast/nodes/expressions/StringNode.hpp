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
        explicit StringNode(const std::string& val, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), value(val) {}

        const std::string& getValue() const { return value; }
        void setValue(const std::string& val) { value = val; }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitStringNode(this);
        }
    };
}
