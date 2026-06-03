#include "AnnotationDeclarationParser.hpp"
#include <cstddef>
#include <cstdint>
#include "utilities/AnnotationParser.hpp"
#include "../token/TokenType.hpp"
#include "../errors/ParseException.hpp"
#include "../errors/DuplicateDeclarationException.hpp"

namespace parser
{
    using namespace ast::nodes::annotations;
    using namespace token;
    using namespace errors;

    AnnotationDeclarationParser::AnnotationDeclarationParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }

    std::unique_ptr<ASTNode> AnnotationDeclarationParser::parse()
    {
        return parseAnnotationDeclaration();
    }

    bool AnnotationDeclarationParser::canParse(const TokenStream& stream) const
    {
        if (stream.current().type == TokenType::ANNOTATION) return true;

        // Also accept `@X ... annotation Name` — skip the leading annotation
        // chain and check whether it lands on the ANNOTATION keyword.
        //
        // This operates on the already-lexed token stream, so string literals
        // and comments cannot corrupt the scan: string contents arrive as a
        // single STRING_LITERAL token, comments are dropped by the lexer. The
        // only tokens we look at here are AT, IDENTIFIER, LPAREN, RPAREN,
        // ANNOTATION, and END — no surface-level ambiguity with source text.
        if (stream.current().type != TokenType::AT) return false;

        size_t lookAhead = 0;
        while (stream.peekAhead(lookAhead).type == TokenType::AT)
        {
            lookAhead++; // '@'
            if (stream.peekAhead(lookAhead).type != TokenType::IDENTIFIER) return false;
            lookAhead++; // name

            if (stream.peekAhead(lookAhead).type == TokenType::LPAREN)
            {
                lookAhead++;
                int depth = 1;
                while (depth > 0 && stream.peekAhead(lookAhead).type != TokenType::END)
                {
                    TokenType t = stream.peekAhead(lookAhead).type;
                    if (t == TokenType::LPAREN) depth++;
                    else if (t == TokenType::RPAREN) depth--;
                    lookAhead++;
                }
                if (depth != 0) return false;
            }
        }
        return stream.peekAhead(lookAhead).type == TokenType::ANNOTATION;
    }

    std::unique_ptr<AnnotationDeclarationNode> AnnotationDeclarationParser::parseAnnotationDeclaration()
    {
        validateDeclarationContext();

        // Consume any leading meta-annotation chain (`@Retention(RUNTIME)
        // @Target([METHOD]) annotation Foo { ... }`).
        std::vector<std::shared_ptr<AnnotationNode>> metaAnnotations;
        if (tokenStream.check(TokenType::AT))
        {
            metaAnnotations = utilities::AnnotationParser::parseAnnotations(tokenStream, &context);
        }

        SourceLocation declLocation = tokenStream.current().location;
        expectToken(TokenType::ANNOTATION);

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            throw ParseException(
                "Expected annotation type name after 'annotation' keyword",
                tokenStream.current().location);
        }

        std::string annotationName = std::string(tokenStream.current().stringValue);
        SourceLocation nameLocation = tokenStream.current().location;
        tokenStream.advance();

        if (context.isTypeDeclared(annotationName))
        {
            SourceLocation firstLocation = context.getTypeDeclarationLocation(annotationName);
            throw DuplicateDeclarationException(
                "annotation",
                annotationName,
                firstLocation,
                nameLocation);
        }
        context.registerTypeName(annotationName);

        expectToken(TokenType::LBRACE);

        auto node = std::make_unique<AnnotationDeclarationNode>(annotationName, declLocation);

        for (auto& meta : metaAnnotations)
        {
            node->addMetaAnnotation(std::move(meta));
        }

        // Field declaration loop: `Type[?] name [= literal];` repeated.
        while (!tokenStream.check(TokenType::RBRACE) && !tokenStream.isAtEnd())
        {
            AnnotationParamDecl decl = parseParamDeclaration();
            // Detect duplicate field names within the same declaration.
            for (const auto& existing : node->getParams())
            {
                if (existing.name == decl.name)
                {
                    throw ParseException(
                        "Duplicate annotation parameter '" + decl.name + "'",
                        tokenStream.current().location);
                }
            }
            node->addParam(std::move(decl));
        }

        expectToken(TokenType::RBRACE);
        return node;
    }

    AnnotationParamDecl AnnotationDeclarationParser::parseParamDeclaration()
    {
        AnnotationParamDecl decl;
        ParsedTypeSpec spec = parseParamType();
        decl.declaredType = spec.type;
        decl.nullable     = spec.nullable;
        decl.isArray      = spec.isArray;

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            throw ParseException(
                "Expected annotation parameter name",
                tokenStream.current().location);
        }
        decl.name = std::string(tokenStream.current().stringValue);
        tokenStream.advance();

        if (tryConsumeToken(TokenType::ASSIGN))
        {
            decl.defaultValue = parseDefaultLiteral(spec);
        }

        expectToken(TokenType::SEMICOLON);
        return decl;
    }

    AnnotationDeclarationParser::ParsedTypeSpec AnnotationDeclarationParser::parseParamType()
    {
        ParsedTypeSpec spec;
        const Token& tok = tokenStream.current();

        switch (tok.type)
        {
        case TokenType::INT:         spec.type = AnnotationValueType::INT;       tokenStream.advance(); break;
        case TokenType::FLOAT:       spec.type = AnnotationValueType::FLOAT;     tokenStream.advance(); break;
        case TokenType::BOOL:        spec.type = AnnotationValueType::BOOL;      tokenStream.advance(); break;
        case TokenType::STRING_TYPE: spec.type = AnnotationValueType::STRING;    tokenStream.advance(); break;
        case TokenType::IDENTIFIER:
            // Only `Class` is accepted as a class-typed annotation parameter for v1.
            if (tok.stringValue != "Class")
            {
                throw ParseException(
                    "Annotation parameter type must be one of: int, float, bool, string, Class, or supported arrays",
                    tok.location);
            }
            spec.type = AnnotationValueType::CLASS_REF;
            tokenStream.advance();
            break;
        default:
            throw ParseException(
                "Expected annotation parameter type (int, float, bool, string, Class, or supported arrays)",
                tok.location);
        }

        // Optional `[]` suffix for compile-time-safe annotation arrays.
        if (tryConsumeToken(TokenType::LBRACKET))
        {
            expectToken(TokenType::RBRACKET);
            spec.isArray = true;
            switch (spec.type)
            {
            case AnnotationValueType::INT:       spec.type = AnnotationValueType::INT_ARRAY; break;
            case AnnotationValueType::FLOAT:     spec.type = AnnotationValueType::FLOAT_ARRAY; break;
            case AnnotationValueType::BOOL:      spec.type = AnnotationValueType::BOOL_ARRAY; break;
            case AnnotationValueType::STRING:    spec.type = AnnotationValueType::STRING_ARRAY; break;
            case AnnotationValueType::CLASS_REF: spec.type = AnnotationValueType::CLASS_ARRAY; break;
            default:
                throw ParseException(
                    "Unsupported annotation array parameter type",
                    tokenStream.current().location);
            }
        }

        // Optional `?` for nullability — applies to reference-typed params.
        if (tryConsumeToken(TokenType::QUESTION))
        {
            if (spec.type == AnnotationValueType::INT  ||
                spec.type == AnnotationValueType::FLOAT ||
                spec.type == AnnotationValueType::BOOL)
            {
                throw ParseException(
                    "Primitive annotation parameters cannot be nullable",
                    tokenStream.current().location);
            }
            spec.nullable = true;
        }
        return spec;
    }

    TypedAnnotationValue AnnotationDeclarationParser::parseDefaultLiteral(const ParsedTypeSpec& spec)
    {
        const Token& tok = tokenStream.current();

        // null is legal only when the parameter is nullable.
        if (tok.type == TokenType::NULL_LITERAL)
        {
            if (!spec.nullable)
            {
                throw ParseException(
                    "Default value 'null' is only allowed on nullable annotation parameters",
                    tok.location);
            }
            tokenStream.advance();
            return TypedAnnotationValue::makeNull();
        }

        switch (spec.type)
        {
        case AnnotationValueType::INT:
        {
            if (tok.type != TokenType::INT_NUMBER)
                throw ParseException("Expected integer literal as default value", tok.location);
            int64_t v = tok.intValue;
            tokenStream.advance();
            return TypedAnnotationValue::makeInt(v);
        }
        case AnnotationValueType::FLOAT:
        {
            if (tok.type == TokenType::FLOAT_NUMBER)
            {
                double v = tok.floatValue;
                tokenStream.advance();
                return TypedAnnotationValue::makeFloat(v);
            }
            if (tok.type == TokenType::INT_NUMBER)
            {
                // Accept integer literal as float default (e.g., `float x = 5`).
                double v = static_cast<double>(tok.intValue);
                tokenStream.advance();
                return TypedAnnotationValue::makeFloat(v);
            }
            throw ParseException("Expected float literal as default value", tok.location);
        }
        case AnnotationValueType::BOOL:
        {
            if (tok.type == TokenType::TRUE)  { tokenStream.advance(); return TypedAnnotationValue::makeBool(true); }
            if (tok.type == TokenType::FALSE) { tokenStream.advance(); return TypedAnnotationValue::makeBool(false); }
            throw ParseException("Expected boolean literal as default value", tok.location);
        }
        case AnnotationValueType::STRING:
        {
            if (tok.type != TokenType::STRING_LITERAL)
                throw ParseException("Expected string literal as default value", tok.location);
            std::string v = std::string(tok.stringValue);
            tokenStream.advance();
            return TypedAnnotationValue::makeString(std::move(v));
        }
        case AnnotationValueType::CLASS_REF:
        {
            if (tok.type != TokenType::IDENTIFIER)
                throw ParseException("Expected class identifier as default value", tok.location);
            std::string v = std::string(tok.stringValue);
            tokenStream.advance();
            return TypedAnnotationValue::makeClassRef(std::move(v));
        }
        case AnnotationValueType::CLASS_ARRAY:
        {
            if (tok.type != TokenType::LBRACKET)
                throw ParseException("Expected '[' to begin Class[] default value", tok.location);
            tokenStream.advance();
            std::vector<std::string> entries;
            while (!tokenStream.check(TokenType::RBRACKET))
            {
                if (!tokenStream.check(TokenType::IDENTIFIER))
                {
                    throw ParseException("Expected class identifier in Class[] default", tokenStream.current().location);
                }
                entries.push_back(std::string(tokenStream.current().stringValue));
                tokenStream.advance();
                if (tokenStream.check(TokenType::COMMA)) tokenStream.advance();
                else if (!tokenStream.check(TokenType::RBRACKET))
                {
                    throw ParseException("Expected ',' or ']' in Class[] default", tokenStream.current().location);
                }
            }
            expectToken(TokenType::RBRACKET);
            return TypedAnnotationValue::makeClassArray(std::move(entries));
        }
        case AnnotationValueType::INT_ARRAY:
        {
            if (tok.type != TokenType::LBRACKET)
                throw ParseException("Expected '[' to begin int[] default value", tok.location);
            tokenStream.advance();
            std::vector<int64_t> entries;
            while (!tokenStream.check(TokenType::RBRACKET))
            {
                if (!tokenStream.check(TokenType::INT_NUMBER))
                {
                    throw ParseException("Expected integer literal in int[] default", tokenStream.current().location);
                }
                entries.push_back(tokenStream.current().intValue);
                tokenStream.advance();
                if (tokenStream.check(TokenType::COMMA)) tokenStream.advance();
                else if (!tokenStream.check(TokenType::RBRACKET))
                {
                    throw ParseException("Expected ',' or ']' in int[] default", tokenStream.current().location);
                }
            }
            expectToken(TokenType::RBRACKET);
            return TypedAnnotationValue::makeIntArray(std::move(entries));
        }
        case AnnotationValueType::FLOAT_ARRAY:
        {
            if (tok.type != TokenType::LBRACKET)
                throw ParseException("Expected '[' to begin float[] default value", tok.location);
            tokenStream.advance();
            std::vector<double> entries;
            while (!tokenStream.check(TokenType::RBRACKET))
            {
                if (tokenStream.check(TokenType::FLOAT_NUMBER))
                {
                    entries.push_back(tokenStream.current().floatValue);
                }
                else if (tokenStream.check(TokenType::INT_NUMBER))
                {
                    entries.push_back(static_cast<double>(tokenStream.current().intValue));
                }
                else
                {
                    throw ParseException("Expected numeric literal in float[] default", tokenStream.current().location);
                }
                tokenStream.advance();
                if (tokenStream.check(TokenType::COMMA)) tokenStream.advance();
                else if (!tokenStream.check(TokenType::RBRACKET))
                {
                    throw ParseException("Expected ',' or ']' in float[] default", tokenStream.current().location);
                }
            }
            expectToken(TokenType::RBRACKET);
            return TypedAnnotationValue::makeFloatArray(std::move(entries));
        }
        case AnnotationValueType::BOOL_ARRAY:
        {
            if (tok.type != TokenType::LBRACKET)
                throw ParseException("Expected '[' to begin bool[] default value", tok.location);
            tokenStream.advance();
            std::vector<bool> entries;
            while (!tokenStream.check(TokenType::RBRACKET))
            {
                if (tokenStream.check(TokenType::TRUE)) entries.push_back(true);
                else if (tokenStream.check(TokenType::FALSE)) entries.push_back(false);
                else throw ParseException("Expected boolean literal in bool[] default", tokenStream.current().location);

                tokenStream.advance();
                if (tokenStream.check(TokenType::COMMA)) tokenStream.advance();
                else if (!tokenStream.check(TokenType::RBRACKET))
                {
                    throw ParseException("Expected ',' or ']' in bool[] default", tokenStream.current().location);
                }
            }
            expectToken(TokenType::RBRACKET);
            return TypedAnnotationValue::makeBoolArray(std::move(entries));
        }
        case AnnotationValueType::STRING_ARRAY:
        {
            if (tok.type != TokenType::LBRACKET)
                throw ParseException("Expected '[' to begin string[] default value", tok.location);
            tokenStream.advance();
            std::vector<std::string> entries;
            while (!tokenStream.check(TokenType::RBRACKET))
            {
                if (!tokenStream.check(TokenType::STRING_LITERAL))
                {
                    throw ParseException("Expected string literal in string[] default", tokenStream.current().location);
                }
                entries.push_back(std::string(tokenStream.current().stringValue));
                tokenStream.advance();
                if (tokenStream.check(TokenType::COMMA)) tokenStream.advance();
                else if (!tokenStream.check(TokenType::RBRACKET))
                {
                    throw ParseException("Expected ',' or ']' in string[] default", tokenStream.current().location);
                }
            }
            expectToken(TokenType::RBRACKET);
            return TypedAnnotationValue::makeStringArray(std::move(entries));
        }
        default:
            throw ParseException("Unsupported annotation parameter type for default value", tok.location);
        }
    }

    void AnnotationDeclarationParser::validateDeclarationContext()
    {
        if (context.isInsideClassBody())
        {
            throw ParseException(
                "Annotation declarations inside class bodies are not allowed.",
                tokenStream.current().location);
        }
        if (context.isInsideInterfaceBody())
        {
            throw ParseException(
                "Annotation declarations inside interface bodies are not allowed.",
                tokenStream.current().location);
        }
        if (context.isInsideFunctionBody())
        {
            throw ParseException(
                "Annotation declarations inside function bodies are not allowed.",
                tokenStream.current().location);
        }
    }
}
