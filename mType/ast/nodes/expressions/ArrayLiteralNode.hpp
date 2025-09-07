#pragma once
#include "../../ASTNode.hpp"
#include <vector>
#include <memory>

namespace ast::nodes::expressions
{
    class ArrayLiteralNode : public ASTNode
    {
    private:
        std::vector<std::unique_ptr<ASTNode>> elements;
        ValueType elementType;

    public:
        explicit ArrayLiteralNode(ValueType elemType, const SourceLocation& loc = SourceLocation());

        void addElement(std::unique_ptr<ASTNode> element);
        const std::vector<std::unique_ptr<ASTNode>>& getElements() const;
        ValueType getElementType() const;
        size_t getElementCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}