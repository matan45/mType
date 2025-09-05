#include "ConstructorNode.hpp"

namespace ast::nodes::classes
{
    // Constructor accepting shared_ptr
    ConstructorNode::ConstructorNode(std::vector<std::pair<std::string, ValueType>> params,
                                     std::shared_ptr<ASTNode> constructorBody,
                                     const SourceLocation& loc)
        : ASTNode(loc), parameters(std::move(params)),
          body(std::move(constructorBody))
    {
    }
    
    // Constructor accepting unique_ptr for backward compatibility
    ConstructorNode::ConstructorNode(std::vector<std::pair<std::string, ValueType>> params,
                                     std::unique_ptr<ASTNode> constructorBody,
                                     const SourceLocation& loc)
        : ASTNode(loc), parameters(std::move(params)),
          body(std::move(constructorBody))  // unique_ptr converts to shared_ptr automatically
    {
    }

    const std::vector<std::pair<std::string, ValueType>>& ConstructorNode::getParameters() const
    {
        return parameters;
    }

    std::shared_ptr<ASTNode> ConstructorNode::getBody() const
    {
        return body;
    }
    
    ASTNode* ConstructorNode::getBodyPtr() const noexcept
    {
        return body.get();
    }

    void ConstructorNode::setParameters(std::vector<std::pair<std::string, ValueType>> params)
    {
        parameters = std::move(params);
    }

    void ConstructorNode::setBody(std::shared_ptr<ASTNode> constructorBody)
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
