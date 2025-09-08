#pragma once
#include "../../ASTNode.hpp"
#include "../../../parser/TypeParser.hpp"
#include <vector>
#include <memory>

namespace ast::nodes::expressions
{
    class MapLiteralNode : public ASTNode
    {
    private:
        std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>> keyValuePairs;
        parser::TypeInfo mapTypeInfo;

    public:
        explicit MapLiteralNode(const parser::TypeInfo& mapTypeInfo, const SourceLocation& loc = SourceLocation());
        
        // Legacy constructor for backward compatibility
        explicit MapLiteralNode(ValueType keyType, ValueType valueType, const SourceLocation& loc = SourceLocation());

        void addKeyValuePair(std::unique_ptr<ASTNode> key, std::unique_ptr<ASTNode> value);
        const std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>>& getKeyValuePairs() const;
        ValueType getKeyType() const;
        ValueType getValueType() const;
        const parser::TypeInfo& getMapTypeInfo() const;
        size_t getPairCount() const;

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}