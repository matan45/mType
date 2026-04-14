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

    Value AnnotationDeclarationNode::accept(ASTVisitor<Value>& visitor)
    {
        return visitor.visitAnnotationDeclarationNode(this);
    }

    std::unique_ptr<ASTNode> AnnotationDeclarationNode::clone() const
    {
        auto copy = std::make_unique<AnnotationDeclarationNode>(name, location);
        for (const auto& p : params) copy->addParam(p);
        return copy;
    }
}
