#include "MapLiteralNode.hpp"

namespace ast::nodes::expressions
{
    MapLiteralNode::MapLiteralNode(ValueType keyType, ValueType valueType, const SourceLocation& loc)
        : ASTNode(loc), keyType(keyType), valueType(valueType) {}

    void MapLiteralNode::addKeyValuePair(std::unique_ptr<ASTNode> key, std::unique_ptr<ASTNode> value)
    {
        keyValuePairs.emplace_back(std::move(key), std::move(value));
    }

    const std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>>& MapLiteralNode::getKeyValuePairs() const
    {
        return keyValuePairs;
    }

    ValueType MapLiteralNode::getKeyType() const
    {
        return keyType;
    }

    ValueType MapLiteralNode::getValueType() const
    {
        return valueType;
    }

    size_t MapLiteralNode::getPairCount() const
    {
        return keyValuePairs.size();
    }

    Value MapLiteralNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitMapLiteralNode(this);
    }
}