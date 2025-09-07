#pragma once
#include "../../ASTNode.hpp"
#include <memory>
#include <string>

namespace ast::nodes::statements
{
    class ForEachNode : public ASTNode
    {
    private:
        std::string variableName;
        ValueType variableType;
        std::unique_ptr<ASTNode> collection;
        std::unique_ptr<ASTNode> body;

    public:
        explicit ForEachNode(const std::string& varName, ValueType varType, 
                           std::unique_ptr<ASTNode> collection, std::unique_ptr<ASTNode> body,
                           const SourceLocation& loc = SourceLocation());

        const std::string& getVariableName() const;
        ValueType getVariableType() const;
        ASTNode* getCollection() const;
        ASTNode* getBody() const;
        
        void setVariableName(const std::string& varName);
        void setVariableType(ValueType varType);
        void setCollection(std::unique_ptr<ASTNode> collection);
        void setBody(std::unique_ptr<ASTNode> body);

        Value accept(ASTVisitor<Value>& visitor) override;
    };
}