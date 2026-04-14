#pragma once

#include "../ast/nodes/annotations/AnnotationNode.hpp"
#include "../runtimeTypes/klass/AnnotationDefinition.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include <memory>

namespace reflection
{
    /// Builds the synthetic ClassDefinition that backs object-style annotation
    /// access (`m.getAnnotation(Test).expected`). Each AnnotationDefinition
    /// gets a one-time synthetic class whose fields mirror the parameter
    /// schema; instances are populated per-call from the AnnotationNode's
    /// typed values.
    class AnnotationInstanceFactory
    {
    public:
        /// Returns (or lazily creates and caches) the synthetic ClassDefinition
        /// associated with `def`. Field names match the parameter schema; field
        /// ValueTypes are mapped from AnnotationValueType via best-effort
        /// (CLASS_REF/CLASS_ARRAY → OBJECT, etc.).
        static std::shared_ptr<runtimeTypes::klass::ClassDefinition>
            getOrCreateSyntheticClass(std::shared_ptr<runtimeTypes::klass::AnnotationDefinition> def);

        /// Builds a fresh ObjectInstance of the synthetic class and populates
        /// each field from the annotation's typed parameters. Class refs are
        /// stored as their textual name (string-typed field) — full Class
        /// resolution happens when callers invoke Class::forName explicitly.
        static std::shared_ptr<runtimeTypes::klass::ObjectInstance>
            buildInstance(std::shared_ptr<runtimeTypes::klass::AnnotationDefinition> def,
                          std::shared_ptr<ast::nodes::annotations::AnnotationNode> annotation);
    };
}
