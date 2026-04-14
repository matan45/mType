#pragma once

#include "AnnotationParamSchema.hpp"
#include "../Definition.hpp"
#include "../../ast/nodes/annotations/AnnotationNode.hpp"
#include <memory>
#include <string>
#include <vector>

namespace runtimeTypes::klass
{
    class ClassDefinition;

    /// Runtime form of a user-declared `annotation Foo { ... }` type.
    /// Carries the ordered parameter schema plus a flag for built-ins that
    /// take a fixed, hard-coded validator path (Override, Script, EntryPoint, Throw).
    class AnnotationDefinition : public Definition
    {
    public:
        explicit AnnotationDefinition(const std::string& name, bool builtin = false)
            : Definition(name), builtin_(builtin) {}

        bool isBuiltin() const { return builtin_; }

        void addParam(AnnotationParamSchema schema) { params_.push_back(std::move(schema)); }
        const std::vector<AnnotationParamSchema>& getParams() const { return params_; }

        const AnnotationParamSchema* findParam(const std::string& paramName) const
        {
            for (const auto& p : params_) if (p.name == paramName) return &p;
            return nullptr;
        }

        // M6 wiring: cache the synthetic class created by AnnotationInstanceFactory
        // so reflection's getAnnotation(annotationClass) can return real ObjectInstances.
        void setSyntheticClass(std::shared_ptr<ClassDefinition> cls) { syntheticClass_ = std::move(cls); }
        std::shared_ptr<ClassDefinition> getSyntheticClass() const { return syntheticClass_; }

        // Meta-annotations (`@X` applied to this annotation's declaration).
        void addMetaAnnotation(std::shared_ptr<ast::nodes::annotations::AnnotationNode> annotation)
        {
            metaAnnotations_.push_back(std::move(annotation));
        }
        const std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>&
            getMetaAnnotations() const { return metaAnnotations_; }
        std::shared_ptr<ast::nodes::annotations::AnnotationNode>
            getMetaAnnotation(const std::string& annotationName) const
        {
            for (const auto& m : metaAnnotations_)
            {
                if (m && m->getName() == annotationName) return m;
            }
            return nullptr;
        }
        bool hasMetaAnnotation(const std::string& annotationName) const
        {
            return getMetaAnnotation(annotationName) != nullptr;
        }

    private:
        std::vector<AnnotationParamSchema> params_;
        bool builtin_;
        std::shared_ptr<ClassDefinition> syntheticClass_;
        std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> metaAnnotations_;
    };
}
