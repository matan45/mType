#include "AnnotationDeclarationNode.hpp"

namespace ast::nodes::annotations
{
    AnnotationDeclarationNode::AnnotationDeclarationNode(const std::string& annotationName,
                                                         const SourceLocation& loc)
        : ASTNode(loc), name(annotationName)
    {
    }

    const std::string& AnnotationDeclarationNode::getName() const
    {
        return name;
    }

    void AnnotationDeclarationNode::addParam(AnnotationParamDecl decl)
    {
        params.push_back(std::move(decl));
    }

    const std::vector<AnnotationParamDecl>& AnnotationDeclarationNode::getParams() const
    {
        return params;
    }

    void AnnotationDeclarationNode::addMetaAnnotation(std::shared_ptr<AnnotationNode> annotation)
    {
        metaAnnotations.push_back(std::move(annotation));
    }

    const std::vector<std::shared_ptr<AnnotationNode>>& AnnotationDeclarationNode::getMetaAnnotations() const
    {
        return metaAnnotations;
    }

    std::shared_ptr<AnnotationNode> AnnotationDeclarationNode::getMetaAnnotation(const std::string& annotationName) const
    {
        for (const auto& meta : metaAnnotations)
        {
            if (meta && meta->getName() == annotationName) return meta;
        }
        return nullptr;
    }

    bool AnnotationDeclarationNode::hasMetaAnnotation(const std::string& annotationName) const
    {
        return getMetaAnnotation(annotationName) != nullptr;
    }

    Value AnnotationDeclarationNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitAnnotationDeclarationNode(this);
    }

    std::unique_ptr<ASTNode> AnnotationDeclarationNode::clone() const
    {
        auto copy = std::make_unique<AnnotationDeclarationNode>(name, location);
        for (const auto& p : params) copy->addParam(p);
        for (const auto& meta : metaAnnotations)
        {
            if (!meta) continue;
            auto cloned = std::unique_ptr<AnnotationNode>(
                static_cast<AnnotationNode*>(meta->clone().release()));
            copy->addMetaAnnotation(std::shared_ptr<AnnotationNode>(std::move(cloned)));
        }
        return copy;
    }
}
