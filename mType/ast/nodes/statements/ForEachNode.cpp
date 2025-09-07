#include "ForEachNode.hpp"

namespace ast::nodes::statements
{
    ForEachNode::ForEachNode(const std::string& varName, ValueType varType, 
                           std::unique_ptr<ASTNode> collection, std::unique_ptr<ASTNode> body,
                           const SourceLocation& loc)
        : ASTNode(loc), variableName(varName), variableType(varType), 
          collection(std::move(collection)), body(std::move(body)) {}

    const std::string& ForEachNode::getVariableName() const
    {
        return variableName;
    }

    ValueType ForEachNode::getVariableType() const
    {
        return variableType;
    }

    ASTNode* ForEachNode::getCollection() const
    {
        return collection.get();
    }

    ASTNode* ForEachNode::getBody() const
    {
        return body.get();
    }

    void ForEachNode::setVariableName(const std::string& varName)
    {
        variableName = varName;
    }

    void ForEachNode::setVariableType(ValueType varType)
    {
        variableType = varType;
    }

    void ForEachNode::setCollection(std::unique_ptr<ASTNode> newCollection)
    {
        collection = std::move(newCollection);
    }

    void ForEachNode::setBody(std::unique_ptr<ASTNode> newBody)
    {
        body = std::move(newBody);
    }

    Value ForEachNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitForEachNode(this);
    }
}