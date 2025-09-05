#include "FunctionNode.hpp"

namespace ast::nodes::functions
{
    FunctionNode::FunctionNode(const std::string& funcName, ValueType retType,
                               std::vector<std::pair<std::string, ValueType>> params, std::shared_ptr<ASTNode> funcBody,
                               const SourceLocation& loc)
        : ASTNode(loc), name(funcName), returnType(retType), parameters(std::move(params)),
          body(std::move(funcBody))
    {
    }

    FunctionNode::FunctionNode(const std::string& funcName, ValueType retType,
                               std::vector<std::pair<std::string, ValueType>> params, std::unique_ptr<ASTNode> funcBody,
                               const SourceLocation& loc)
        : ASTNode(loc), name(funcName), returnType(retType), parameters(std::move(params)),
          body(std::move(funcBody)) // unique_ptr converts to shared_ptr
    {
    }

    const std::string& FunctionNode::getName() const
    {
        return name;
    }

    ValueType FunctionNode::getReturnType() const
    {
        return returnType;
    }

    const std::vector<std::pair<std::string, ValueType>>& FunctionNode::getParameters() const
    {
        return parameters;
    }

    std::shared_ptr<ASTNode> FunctionNode::getBody() const
    {
        return body;
    }

    ASTNode* FunctionNode::getBodyPtr() const noexcept
    {
        return body.get();
    }

    void FunctionNode::setName(const std::string& funcName)
    {
        name = funcName;
    }

    void FunctionNode::setReturnType(ValueType retType)
    {
        returnType = retType;
    }

    void FunctionNode::setParameters(std::vector<std::pair<std::string, ValueType>> params)
    {
        parameters = std::move(params);
    }

    void FunctionNode::setBody(std::shared_ptr<ASTNode> funcBody)
    {
        body = std::move(funcBody);
    }

    size_t FunctionNode::getParameterCount() const
    {
        return parameters.size();
    }

    Value FunctionNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitFunctionNode(this);
    }
}
