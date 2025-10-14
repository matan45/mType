#include "ForEachNode.hpp"

namespace ast::nodes::statements
{
    ForEachNode::ForEachNode(const std::string& varName, const parser::TypeInfo& varTypeInfo, 
                           std::unique_ptr<ASTNode> collection, std::unique_ptr<ASTNode> body,
                           const SourceLocation& loc)
        : ASTNode(loc), variableName(varName), variableTypeInfo(varTypeInfo), 
          collection(std::move(collection)), body(std::move(body)) {}

    // Legacy constructor for backward compatibility
    ForEachNode::ForEachNode(const std::string& varName, ValueType varType, 
                           std::unique_ptr<ASTNode> collection, std::unique_ptr<ASTNode> body,
                           const SourceLocation& loc)
        : ASTNode(loc), variableName(varName), variableTypeInfo(parser::TypeInfo(varType)), 
          collection(std::move(collection)), body(std::move(body)) {}

    const std::string& ForEachNode::getVariableName() const
    {
        return variableName;
    }

    ValueType ForEachNode::getVariableType() const
    {
        return variableTypeInfo.baseType;
    }

    const parser::TypeInfo& ForEachNode::getVariableTypeInfo() const
    {
        return variableTypeInfo;
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
        variableTypeInfo = parser::TypeInfo(varType);
    }

    void ForEachNode::setVariableTypeInfo(const parser::TypeInfo& varTypeInfo)
    {
        variableTypeInfo = varTypeInfo;
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

    std::unique_ptr<ASTNode> ForEachNode::clone() const
    {
        std::unique_ptr<ASTNode> clonedCollection = collection ? collection->clone() : nullptr;
        std::unique_ptr<ASTNode> clonedBody = body ? body->clone() : nullptr;

        return std::make_unique<ForEachNode>(variableName, variableTypeInfo,
                                             std::move(clonedCollection), std::move(clonedBody), location);
    }
}