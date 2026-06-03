#pragma once

#include "../../ast/nodes/annotations/AnnotationNode.hpp"
#include "../../ast/nodes/annotations/TypedAnnotationValue.hpp"
#include "../TokenStream.hpp"
#include "../../token/TokenType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <optional>
#include <vector>
#include <memory>

namespace parser
{
    class ParseContext; // MYT-376: needed to parse constant-expression arguments
}

namespace parser::utilities
{
    using namespace ast::nodes::annotations;
    using namespace token;
    using namespace errors;

    /// Parses `@AnnotationName[(args)]` annotation usages.
    /// `args` accepts:
    ///   * named pairs: `key = value, key2 = value2`
    ///   * positional shorthand: a single bare value — semantically bound to
    ///     the sole declared parameter (validator enforces arity)
    ///   * legacy bare-identifier list — preserved for the @Throw migration
    ///     path; identifiers parsed as an ordered list of CLASS_REF values.
    /// Value forms: int, float, bool (true/false), string, identifier
    /// (treated as a Class reference), null, and homogeneous array literals.
    /// MYT-376: a value may also be a compile-time constant expression
    /// (arithmetic / string concat / unary / ternary / cast) or a
    /// `Class::FIELD` static-final constant reference. Such values cannot be
    /// folded at parse time (they may reference classes parsed later or
    /// imported), so they are stored as a deferred expression AST on the
    /// AnnotationNode and folded by AnnotationConstantResolver before bytecode
    /// serialization. A bare identifier (no `::`) stays a CLASS_REF, preserving
    /// `@Throw(IOExc)` / `@Target([METHOD])` semantics.
    class AnnotationParser
    {
    public:
        // `context` is required to parse constant-expression arguments. When
        // null (e.g. the parameter-annotation path) expression-valued
        // arguments are rejected with a clear parse error; plain literals,
        // class refs, and literal arrays still work.
        static std::shared_ptr<AnnotationNode> parseAnnotation(TokenStream& tokenStream,
                                                               ParseContext* context = nullptr);
        static std::vector<std::shared_ptr<AnnotationNode>> parseAnnotations(TokenStream& tokenStream,
                                                                             ParseContext* context = nullptr);
        static bool isAnnotation(TokenType type);

    private:
        // A parsed annotation value: either a fully-known literal
        // (TypedAnnotationValue) or a deferred constant-expression AST node to
        // be folded later by AnnotationConstantResolver.
        struct ParsedValue
        {
            std::optional<TypedAnnotationValue> value;
            std::unique_ptr<ast::ASTNode> deferred;

            bool isDeferred() const { return deferred != nullptr; }

            static ParsedValue ofValue(TypedAnnotationValue v)
            {
                ParsedValue p;
                p.value = std::move(v);
                return p;
            }
            static ParsedValue ofDeferred(std::unique_ptr<ast::ASTNode> d)
            {
                ParsedValue p;
                p.deferred = std::move(d);
                return p;
            }
        };

        // Top-level argument parser — handles `(key = value, ...)` or shorthand.
        static void parseAnnotationArguments(TokenStream& tokenStream, AnnotationNode& target,
                                             ParseContext* context);

        // Parses a single value at the current token (literal, class ref,
        // constant expression, or array literal), advancing past it.
        static ParsedValue parseAnnotationValue(TokenStream& tokenStream, ParseContext* context);

        // Parses an array literal. Returns a literal array value when every
        // element is a plain literal, or a deferred ArrayLiteralNode when any
        // element is a constant expression / `Class::FIELD` reference.
        static ParsedValue parseArrayLiteral(TokenStream& tokenStream, ParseContext* context);

        // Builds a homogeneous literal array from already-parsed plain values,
        // reproducing the original element-type homogeneity checks/messages.
        static TypedAnnotationValue buildLiteralArray(std::vector<ParsedValue>& elements,
                                                      const SourceLocation& loc);
    };
}
