#pragma once
#include "../../ASTNode.hpp"
#include "../../../parser/TypeParser.hpp"
#include <vector>
#include <memory>

namespace ast::nodes::expressions
{
    class ArrayLiteralNode : public ASTNode
    {
    private:
        std::vector<std::unique_ptr<ASTNode>> elements;
        parser::TypeInfo elementTypeInfo;

    public:
        explicit ArrayLiteralNode(const parser::TypeInfo& elemTypeInfo, const SourceLocation& loc = SourceLocation());
        
        // Legacy constructor for backward compatibility
        explicit ArrayLiteralNode(ValueType elemType, const SourceLocation& loc = SourceLocation());

        void addElement(std::unique_ptr<ASTNode> element);
        const std::vector<std::unique_ptr<ASTNode>>& getElements() const;
        ValueType getElementType() const;
        const parser::TypeInfo& getElementTypeInfo() const;
        size_t getElementCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}