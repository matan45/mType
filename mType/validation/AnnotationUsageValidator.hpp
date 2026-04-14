#pragma once

#include "../ast/nodes/annotations/AnnotationNode.hpp"
#include "../environment/Environment.hpp"
#include "../errors/SourceLocation.hpp"
#include <memory>

namespace validation
{
    /// The kind of declaration an annotation is being applied to. Used by
    /// `validate()` to enforce the built-in `@Target` meta-annotation.
    enum class AnnotationHostKind
    {
        UNSPECIFIED,
        CLASS,
        METHOD,
        FIELD,
        CONSTRUCTOR,
        FUNCTION,
        ANNOTATION_DECLARATION
    };

    /// Validates a single `@Annotation(args)` usage against its declared
    /// AnnotationDefinition. Errors:
    ///   * unknown annotation name (no AnnotationDefinition registered)
    ///   * unknown parameter name
    ///   * type mismatch between literal and declared parameter type
    ///   * null on non-nullable parameter
    ///   * missing required parameter
    ///   * duplicate parameter (already caught by parser, but defensive)
    ///   * positional shorthand on annotation with != 1 declared params
    ///   * host kind not in the declaration's @Target set (MYT-109)
    /// Side effect: rewrites positional shorthand to its named binding,
    /// and fills in defaults so reflection sees a complete value set.
    class AnnotationUsageValidator
    {
    public:
        static void validate(
            std::shared_ptr<ast::nodes::annotations::AnnotationNode> annotation,
            std::shared_ptr<environment::Environment> environment,
            const errors::SourceLocation& location,
            AnnotationHostKind hostKind = AnnotationHostKind::UNSPECIFIED);
    };
}
