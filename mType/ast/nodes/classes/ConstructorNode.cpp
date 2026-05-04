#include "ConstructorNode.hpp"
#include <cstddef>
#include "SuperConstructorCallNode.hpp"

namespace ast::nodes::classes
{
    // Constructor with ParameterType (preserves class/interface information)
    ConstructorNode::ConstructorNode(std::vector<std::pair<std::string, ParameterType>> params,
                                     std::shared_ptr<ASTNode> constructorBody,
                                     AccessModifier modifier,
                                     const SourceLocation& loc)
        : ASTNode(loc), parametersWithTypes(std::move(params)),
          body(std::move(constructorBody)), accessModifier(modifier),
          parameterAnnotations(parametersWithTypes.size())
    {
        // No dual storage - parametersWithTypes is the single source of truth
    }

    std::vector<std::pair<std::string, ValueType>> ConstructorNode::getParameters() const
    {
        // Compute on-demand for thread-safety (removed unsafe cache)
        std::vector<std::pair<std::string, ValueType>> parameters;
        parameters.reserve(parametersWithTypes.size());
        for (const auto& [name, paramType] : parametersWithTypes) {
            parameters.emplace_back(name, paramType.basicType);
        }
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

    const std::vector<std::vector<std::shared_ptr<annotations::AnnotationNode>>>&
    ConstructorNode::getParameterAnnotations() const
    {
        return parameterAnnotations;
    }

    void ConstructorNode::setParameterAnnotations(
        std::vector<std::vector<std::shared_ptr<annotations::AnnotationNode>>> annotationsByIndex)
    {
        parameterAnnotations = std::move(annotationsByIndex);
        parameterAnnotations.resize(parametersWithTypes.size());
    }

    const std::vector<std::shared_ptr<annotations::AnnotationNode>>&
    ConstructorNode::getParameterAnnotations(size_t paramIndex) const
    {
        static const std::vector<std::shared_ptr<annotations::AnnotationNode>> empty;
        if (paramIndex >= parameterAnnotations.size()) return empty;
        return parameterAnnotations[paramIndex];
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

        // MYT-108: clone annotations via typed-parameter path
        auto cloneAnn = [](const std::shared_ptr<annotations::AnnotationNode>& annotation)
            -> std::shared_ptr<annotations::AnnotationNode>
        {
            if (!annotation) return nullptr;
            auto cloned = std::make_shared<annotations::AnnotationNode>(
                annotation->getName(), annotation->getLocation());
            for (const auto& key : annotation->getKeyOrder()) {
                if (auto* v = annotation->getTypedParameter(key)) {
                    cloned->setTypedParameter(key, *v);
                }
            }
            return cloned;
        };
        for (const auto& annotation : annotations) {
            if (auto c = cloneAnn(annotation)) clonedConstructor->addAnnotation(c);
        }

        // MYT-110: clone per-parameter annotations
        std::vector<std::vector<std::shared_ptr<annotations::AnnotationNode>>> clonedParamAnnots;
        clonedParamAnnots.reserve(parameterAnnotations.size());
        for (const auto& perParam : parameterAnnotations) {
            std::vector<std::shared_ptr<annotations::AnnotationNode>> out;
            out.reserve(perParam.size());
            for (const auto& a : perParam) {
                if (auto c = cloneAnn(a)) out.push_back(std::move(c));
            }
            clonedParamAnnots.push_back(std::move(out));
        }
        clonedConstructor->setParameterAnnotations(std::move(clonedParamAnnots));

        return clonedConstructor;
    }
}
