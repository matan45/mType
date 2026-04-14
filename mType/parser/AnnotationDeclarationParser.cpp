#include "AnnotationDeclarationParser.hpp"
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
        return stream.current().type == TokenType::ANNOTATION;
    }

    std::unique_ptr<AnnotationDeclarationNode> AnnotationDeclarationParser::parseAnnotationDeclaration()
    {
        validateDeclarationContext();

        SourceLocation declLocation = tokenStream.current().location;
        expectToken(TokenType::ANNOTATION);

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            throw ParseException(
                "Expected annotation type name after 'annotation' keyword",
                tokenStream.current().location);
        }

        std::string annotationName = tokenStream.current().stringValue.getString();
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
        decl.name = tokenStream.current().stringValue.getString();
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
            if (tok.stringValue.getString() != "Class")
            {
                throw ParseException(
                    "Annotation parameter type must be one of: int, float, bool, string, Class, Class[]",
                    tok.location);
            }
            spec.type = AnnotationValueType::CLASS_REF;
            tokenStream.advance();
            break;
        default:
            throw ParseException(
                "Expected annotation parameter type (int, float, bool, string, Class, Class[])",
                tok.location);
        }

        // Optional `[]` suffix — limited to `Class[]` for v1 (per @Throw migration).
        if (tryConsumeToken(TokenType::LBRACKET))
        {
            expectToken(TokenType::RBRACKET);
            if (spec.type != AnnotationValueType::CLASS_REF)
            {
                throw ParseException(
                    "Array-valued annotation parameters are only supported for Class[] in v1",
                    tokenStream.current().location);
            }
            spec.isArray = true;
            spec.type = AnnotationValueType::CLASS_ARRAY;
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
            std::string v = tok.stringValue.getString();
            tokenStream.advance();
            return TypedAnnotationValue::makeString(std::move(v));
        }
        case AnnotationValueType::CLASS_REF:
        {
            if (tok.type != TokenType::IDENTIFIER)
                throw ParseException("Expected class identifier as default value", tok.location);
            std::string v = tok.stringValue.getString();
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
                entries.push_back(tokenStream.current().stringValue.getString());
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
