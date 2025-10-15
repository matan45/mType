#include "ConstructorNode.hpp"
#include "SuperConstructorCallNode.hpp"

namespace ast::nodes::classes
{
    // Constructor with ParameterType (preserves class/interface information)
    ConstructorNode::ConstructorNode(std::vector<std::pair<std::string, ParameterType>> params,
                                     std::shared_ptr<ASTNode> constructorBody,
                                     AccessModifier modifier,
                                     const SourceLocation& loc)
        : ASTNode(loc), parametersWithTypes(std::move(params)),
          body(std::move(constructorBody)), accessModifier(modifier)
    {
        // No dual storage - parametersWithTypes is the single source of truth
    }

    const std::vector<std::pair<std::string, ValueType>>& ConstructorNode::getParameters() const
    {
        // Lazy computation - only compute when needed
        if (!parametersCacheValid) {
            cachedParameters.clear();
            cachedParameters.reserve(parametersWithTypes.size());
            for (const auto& [name, paramType] : parametersWithTypes) {
                cachedParameters.emplace_back(name, paramType.basicType);
            }
            parametersCacheValid = true;
        }
        return cachedParameters;
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

    ASTNode* ConstructorNode::getBodyPtr() const
    {
        return body.get();
    }

    void ConstructorNode::setBody(std::shared_ptr<ASTNode> constructorBody)
    {
        body = std::move(constructorBody);
    }

    void ConstructorNode::setSuperInitializer(std::unique_ptr<SuperConstructorCallNode> superCall)
    {
        superInitializer = std::move(superCall);
    }

    SuperConstructorCallNode* ConstructorNode::getSuperInitializer() const noexcept
    {
        return superInitializer.get();
    }

    bool ConstructorNode::hasSuperInitializer() const noexcept
    {
        return superInitializer != nullptr;
    }

    AccessModifier ConstructorNode::getAccessModifier() const
    {
        return accessModifier;
    }

    void ConstructorNode::setAccessModifier(AccessModifier modifier)
    {
        accessModifier = modifier;
    }

    size_t ConstructorNode::getParameterCount() const
    {
        return parametersWithTypes.size();
    }

    Value ConstructorNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitConstructorNode(this);
    }

    std::unique_ptr<ASTNode> ConstructorNode::clone() const
    {
        // Clone the body (shared_ptr -> unique_ptr for clone, then convert back to shared_ptr)
        std::shared_ptr<ASTNode> clonedBody = body ? std::shared_ptr<ASTNode>(body->clone()) : nullptr;

        // Clone parameters with types (ParameterType has copy constructor)
        std::vector<std::pair<std::string, ParameterType>> clonedParams = parametersWithTypes;

        auto clonedConstructor = std::make_unique<ConstructorNode>(
            clonedParams, clonedBody, accessModifier, location
        );

        // Clone super initializer if present
        if (superInitializer) {
            auto clonedSuper = std::unique_ptr<SuperConstructorCallNode>(
                static_cast<SuperConstructorCallNode*>(superInitializer->clone().release())
            );
            clonedConstructor->setSuperInitializer(std::move(clonedSuper));
        }

        return clonedConstructor;
    }
}
