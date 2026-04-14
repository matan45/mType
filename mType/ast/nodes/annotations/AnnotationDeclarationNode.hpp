#pragma once
#include "../../ASTNode.hpp"
#include "AnnotationNode.hpp"
#include "TypedAnnotationValue.hpp"
#include <string>
#include <memory>
#include <optional>
#include <vector>

namespace ast::nodes::annotations
{
    /// AST-level declaration of a single annotation parameter.
    /// Mirrors runtimeTypes::klass::AnnotationParamSchema but kept in the
    /// AST namespace to keep ast/ free of runtime dependencies.
    struct AnnotationParamDecl
    {
        std::string name;
        AnnotationValueType declaredType;
        std::optional<TypedAnnotationValue> defaultValue;
        bool nullable = false;
        bool isArray  = false;
    };

    class AnnotationDeclarationNode : public ASTNode
    {
    private:
        std::string name;
        std::vector<AnnotationParamDecl> params;
        // Meta-annotations applied to this annotation declaration
        // (e.g. `@Retention(RUNTIME) @Target([METHOD]) annotation Foo { }`).
        std::vector<std::shared_ptr<AnnotationNode>> metaAnnotations;

    public:
        explicit AnnotationDeclarationNode(const std::string& annotationName,
                                           const SourceLocation& loc = SourceLocation());

        const std::string& getName() const;

        void addParam(AnnotationParamDecl decl);
        const std::vector<AnnotationParamDecl>& getParams() const;

        void addMetaAnnotation(std::shared_ptr<AnnotationNode> annotation);
        const std::vector<std::shared_ptr<AnnotationNode>>& getMetaAnnotations() const;
        std::shared_ptr<AnnotationNode> getMetaAnnotation(const std::string& annotationName) const;
        bool hasMetaAnnotation(const std::string& annotationName) const;

        Value accept(ASTVisitor<Value>& visitor) override;
        std::unique_ptr<ASTNode> clone() const override;
    };
}
