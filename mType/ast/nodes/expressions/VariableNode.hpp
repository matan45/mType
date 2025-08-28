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
        explicit VariableNode(const std::string& varName, const SourceLocation& loc = SourceLocation())
            : ASTNode(loc), name(varName) {}

        const std::string& getName() const { return name; }
        void setName(const std::string& varName) { name = varName; }

        Value accept(ASTVisitor<Value>& visitor) override {
            return visitor.visitVariableNode(this);
        }
    };
}
