#include "MapLiteralNode.hpp"

namespace ast::nodes::expressions
{
    MapLiteralNode::MapLiteralNode(const parser::TypeInfo& mapTypeInfo, const SourceLocation& loc)
        : ASTNode(loc), mapTypeInfo(mapTypeInfo) {}
    
    // Legacy constructor for backward compatibility  
    MapLiteralNode::MapLiteralNode(ValueType keyType, ValueType valueType, const SourceLocation& loc)
        : ASTNode(loc), mapTypeInfo(parser::TypeInfo(ValueType::MAP, keyType, valueType)) {}

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
        return mapTypeInfo.keyType.value_or(ValueType::OBJECT);
    }

    ValueType MapLiteralNode::getValueType() const
    {
        return mapTypeInfo.valueType.value_or(ValueType::OBJECT);
    }

    const parser::TypeInfo& MapLiteralNode::getMapTypeInfo() const
    {
        return mapTypeInfo;
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