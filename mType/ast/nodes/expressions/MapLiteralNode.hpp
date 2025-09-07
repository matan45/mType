#pragma once
#include "../../ASTNode.hpp"
#include <vector>
#include <memory>

namespace ast::nodes::expressions
{
    class MapLiteralNode : public ASTNode
    {
    private:
        std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>> keyValuePairs;
        ValueType keyType;
        ValueType valueType;

    public:
        explicit MapLiteralNode(ValueType keyType, ValueType valueType, const SourceLocation& loc = SourceLocation());

        void addKeyValuePair(std::unique_ptr<ASTNode> key, std::unique_ptr<ASTNode> value);
        const std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>>& getKeyValuePairs() const;
        ValueType getKeyType() const;
        ValueType getValueType() const;
        size_t getPairCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}