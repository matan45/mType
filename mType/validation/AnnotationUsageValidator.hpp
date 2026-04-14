#pragma once

#include "../ast/nodes/annotations/AnnotationNode.hpp"
#include "../environment/Environment.hpp"
#include "../errors/SourceLocation.hpp"
#include <memory>

namespace validation
{
    /// Validates a single `@Annotation(args)` usage against its declared
    /// AnnotationDefinition. Errors:
    ///   * unknown annotation name (no AnnotationDefinition registered)
    ///   * unknown parameter name
    ///   * type mismatch between literal and declared parameter type
    ///   * null on non-nullable parameter
    ///   * missing required parameter
    ///   * duplicate parameter (already caught by parser, but defensive)
    ///   * positional shorthand on annotation with != 1 declared params
    /// Side effect: rewrites positional shorthand to its named binding,
    /// and fills in defaults so reflection sees a complete value set.
    class AnnotationUsageValidator
    {
    public:
        static void validate(
            std::shared_ptr<ast::nodes::annotations::AnnotationNode> annotation,
            std::shared_ptr<environment::Environment> environment,
            const errors::SourceLocation& location);
    };
}
