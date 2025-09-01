#include "ConstructorNode.hpp"

namespace ast::nodes::classes
{
    ConstructorNode::ConstructorNode(std::vector<std::pair<std::string, ValueType>> params,
                                     std::unique_ptr<ASTNode> constructorBody,
                                     const SourceLocation& loc)
        : ASTNode(loc), parameters(std::move(params)),
          body(std::move(constructorBody))
    {
    }

    const std::vector<std::pair<std::string, ValueType>>& ConstructorNode::getParameters() const
    {
        return parameters;
    }

    ASTNode* ConstructorNode::getBody() const
    {
        return body.get();
    }

    void ConstructorNode::setParameters(std::vector<std::pair<std::string, ValueType>> params)
    {
        parameters = std::move(params);
    }

    void ConstructorNode::setBody(std::unique_ptr<ASTNode> constructorBody)
    {
        body = std::move(constructorBody);
    }

    size_t ConstructorNode::getParameterCount() const
    {
        return parameters.size();
    }

    Value ConstructorNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitConstructorNode(this);
    }
}
