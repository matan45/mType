#include "ConstructorNode.hpp"

namespace ast::nodes::classes
{
    // Constructor accepting shared_ptr (old format)
    ConstructorNode::ConstructorNode(std::vector<std::pair<std::string, ValueType>> params,
                                     std::shared_ptr<ASTNode> constructorBody,
                                     const SourceLocation& loc)
        : ASTNode(loc), parameters(std::move(params)),
          body(std::move(constructorBody))
    {
    }

    // Constructor accepting unique_ptr for backward compatibility (old format)
    ConstructorNode::ConstructorNode(std::vector<std::pair<std::string, ValueType>> params,
                                     std::unique_ptr<ASTNode> constructorBody,
                                     const SourceLocation& loc)
        : ASTNode(loc), parameters(std::move(params)),
          body(std::move(constructorBody)) // unique_ptr converts to shared_ptr automatically
    {
    }

    // NEW: Constructor with ParameterType (preserves class/interface information)
    ConstructorNode::ConstructorNode(std::vector<std::pair<std::string, ParameterType>> params,
                                     std::shared_ptr<ASTNode> constructorBody,
                                     const SourceLocation& loc)
        : ASTNode(loc), parametersWithTypes(std::move(params)),
          body(std::move(constructorBody))
    {
        // Also populate the old parameters format for backward compatibility
        for (const auto& param : parametersWithTypes) {
            parameters.emplace_back(param.first, param.second.basicType);
        }
    }

    const std::vector<std::pair<std::string, ValueType>>& ConstructorNode::getParameters() const
    {
        return parameters;
    }

    const std::vector<std::pair<std::string, ParameterType>>& ConstructorNode::getParametersWithTypes() const
    {
        return parametersWithTypes;
    }

    bool ConstructorNode::hasParametersWithTypes() const
    {
        return !parametersWithTypes.empty();
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
