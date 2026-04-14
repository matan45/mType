#pragma once

#include "../../ast/nodes/annotations/AnnotationNode.hpp"
#include "../../ast/nodes/annotations/TypedAnnotationValue.hpp"
#include "../TokenStream.hpp"
#include "../../token/TokenType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <vector>
#include <memory>

namespace parser::utilities
{
    using namespace ast::nodes::annotations;
    using namespace token;
    using namespace errors;

    /// Parses `@AnnotationName[(args)]` annotation usages.
    /// `args` accepts:
    ///   * named pairs: `key = literal, key2 = literal2`
    ///   * positional shorthand: a single bare literal — semantically bound to
    ///     the sole declared parameter (validator enforces arity)
    ///   * legacy bare-identifier list — preserved for the @Throw migration
    ///     path; identifiers parsed as an ordered list of CLASS_REF values.
    /// Literal types: int, float, bool (true/false), string, identifier
    /// (treated as a Class reference), null, array literal `[Id, Id, ...]`.
    class AnnotationParser
    {
    public:
        static std::shared_ptr<AnnotationNode> parseAnnotation(TokenStream& tokenStream);
        static std::vector<std::shared_ptr<AnnotationNode>> parseAnnotations(TokenStream& tokenStream);
        static bool isAnnotation(TokenType type);

    private:
        // Top-level argument parser — handles `(key = lit, ...)` or shorthand.
        static void parseAnnotationArguments(TokenStream& tokenStream, AnnotationNode& target);

        // Parses a single literal value at the current token, advancing past it.
        static TypedAnnotationValue parseLiteral(TokenStream& tokenStream);

        // Parses an array literal `[Id, Id, ...]` (Class[] only in v1).
        static TypedAnnotationValue parseArrayLiteral(TokenStream& tokenStream);
    };
}
