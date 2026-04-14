#pragma once

#include "core/BaseParser.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"
#include "../ast/nodes/annotations/AnnotationDeclarationNode.hpp"
#include <memory>
#include <optional>

namespace parser
{
    /// Parses `annotation Name { Type[?] field [= literal]; ... }`.
    /// Supported field types: int, float, bool, string, Class, Class[].
    /// Defaults must be literal expressions of the matching declared type.
    class AnnotationDeclarationParser : public core::BaseParser
    {
    public:
        explicit AnnotationDeclarationParser(TokenStream& stream, ParseContext& ctx);
        ~AnnotationDeclarationParser() override = default;

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        std::unique_ptr<ast::nodes::annotations::AnnotationDeclarationNode>
            parseAnnotationDeclaration();

    private:
        void validateDeclarationContext();

        // Parses a single `Type[?] name [= literal];` field declaration.
        ast::nodes::annotations::AnnotationParamDecl parseParamDeclaration();

        // Parses the type prefix (int / float / bool / string / Class / Class[])
        // and returns the resulting AnnotationValueType plus nullable/isArray flags.
        struct ParsedTypeSpec
        {
            ast::nodes::annotations::AnnotationValueType type;
            bool nullable = false;
            bool isArray  = false;
        };
        ParsedTypeSpec parseParamType();

        // Parses a literal default value, producing a TypedAnnotationValue.
        // The expected type drives literal disambiguation (e.g., `null` only
        // legal when nullable). Throws ParseException on type mismatch.
        ast::nodes::annotations::TypedAnnotationValue parseDefaultLiteral(const ParsedTypeSpec& spec);
    };
}
