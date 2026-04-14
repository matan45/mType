#include "AnnotationParser.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::utilities
{
    std::shared_ptr<AnnotationNode> AnnotationParser::parseAnnotation(TokenStream& tokenStream)
    {
        if (!isAnnotation(tokenStream.current().type))
        {
            return nullptr;
        }

        tokenStream.advance(); // consume '@'

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected annotation name after '@'", tokenStream.current().location);
        }

        std::string annotationName = tokenStream.current().stringValue.getString();
        SourceLocation annotationLocation = tokenStream.current().location;
        tokenStream.advance();

        auto node = std::make_shared<AnnotationNode>(annotationName, annotationLocation);

        if (tokenStream.current().type == TokenType::LPAREN)
        {
            parseAnnotationArguments(tokenStream, *node);
        }

        return node;
    }

    std::vector<std::shared_ptr<AnnotationNode>> AnnotationParser::parseAnnotations(TokenStream& tokenStream)
    {
        std::vector<std::shared_ptr<AnnotationNode>> annotations;
        while (isAnnotation(tokenStream.current().type))
        {
            auto annotation = parseAnnotation(tokenStream);
            if (!annotation)
            {
                throw ParseException("Internal error: parseAnnotation returned null after isAnnotation passed",
                                     tokenStream.current().location);
            }
            annotations.push_back(annotation);
        }
        return annotations;
    }

    bool AnnotationParser::isAnnotation(TokenType type)
    {
        return type == TokenType::AT;
    }

    void AnnotationParser::parseAnnotationArguments(TokenStream& tokenStream, AnnotationNode& target)
    {
        SourceLocation lparenLoc = tokenStream.current().location;
        tokenStream.advance(); // consume '('

        // Empty `()` is allowed — the validator enforces required-parameter checks
        // against the AnnotationDefinition.
        if (tokenStream.current().type == TokenType::RPAREN)
        {
            tokenStream.advance();
            return;
        }

        // Disambiguate:
        //   IDENT = literal       -> named argument
        //   IDENT , IDENT [, ...] -> legacy bare-identifier list (e.g. @Throw(IO,Net))
        //   anything else         -> positional shorthand (single literal)
        bool firstIsIdent  = (tokenStream.current().type == TokenType::IDENTIFIER);
        bool secondIsAssign =
            firstIsIdent && (tokenStream.peek().type == TokenType::ASSIGN);
        bool legacyIdentList =
            firstIsIdent && !secondIsAssign &&
            (tokenStream.peek().type == TokenType::COMMA ||
             tokenStream.peek().type == TokenType::RPAREN);

        if (legacyIdentList)
        {
            // Collect bare identifiers as a Class[]-style ordered list and
            // store under the legacy `"exceptions"` key so unmigrated readers
            // (currently @Throw validator) keep functioning.
            std::vector<std::string> classNames;
            while (tokenStream.current().type != TokenType::RPAREN)
            {
                if (tokenStream.current().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected class identifier in annotation argument list",
                                         tokenStream.current().location);
                }
                classNames.push_back(tokenStream.current().stringValue.getString());
                tokenStream.advance();
                if (tokenStream.current().type == TokenType::COMMA)
                {
                    tokenStream.advance();
                    if (tokenStream.current().type == TokenType::RPAREN)
                    {
                        throw ParseException("Trailing ',' in annotation argument list",
                                             tokenStream.current().location);
                    }
                }
                else if (tokenStream.current().type != TokenType::RPAREN)
                {
                    throw ParseException("Expected ',' or ')' in annotation argument list",
                                         tokenStream.current().location);
                }
            }
            tokenStream.advance(); // consume ')'

            if (classNames.empty())
            {
                throw ParseException("Empty annotation parameters not allowed in legacy form",
                                     lparenLoc);
            }
            // Legacy compatibility key — also exposed to the new validator via
            // typed value of CLASS_ARRAY type.
            target.setTypedParameter("exceptions",
                TypedAnnotationValue::makeClassArray(std::move(classNames)));
            return;
        }

        if (secondIsAssign)
        {
            // Named-argument form: key = literal, key = literal, ...
            while (tokenStream.current().type != TokenType::RPAREN)
            {
                if (tokenStream.current().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected annotation parameter name",
                                         tokenStream.current().location);
                }
                std::string key = tokenStream.current().stringValue.getString();
                SourceLocation keyLoc = tokenStream.current().location;
                tokenStream.advance();

                if (tokenStream.current().type != TokenType::ASSIGN)
                {
                    throw ParseException("Expected '=' after annotation parameter name",
                                         tokenStream.current().location);
                }
                tokenStream.advance();

                if (target.hasParameter(key))
                {
                    throw ParseException("Duplicate annotation parameter '" + key + "'", keyLoc);
                }
                target.setTypedParameter(key, parseLiteral(tokenStream));

                if (tokenStream.current().type == TokenType::COMMA)
                {
                    tokenStream.advance();
                    if (tokenStream.current().type == TokenType::RPAREN)
                    {
                        throw ParseException("Trailing ',' in annotation argument list",
                                             tokenStream.current().location);
                    }
                }
                else if (tokenStream.current().type != TokenType::RPAREN)
                {
                    throw ParseException("Expected ',' or ')' after annotation argument",
                                         tokenStream.current().location);
                }
            }
            tokenStream.advance(); // consume ')'
            return;
        }

        // Positional shorthand: single literal. Semantic binding to the sole
        // declared parameter happens in the usage validator (M4) — here we
        // just stash it under the reserved key "__positional__".
        TypedAnnotationValue value = parseLiteral(tokenStream);
        target.setTypedParameter("__positional__", std::move(value));

        if (tokenStream.current().type != TokenType::RPAREN)
        {
            throw ParseException("Expected ')' after positional annotation argument",
                                 tokenStream.current().location);
        }
        tokenStream.advance(); // consume ')'
    }

    TypedAnnotationValue AnnotationParser::parseLiteral(TokenStream& tokenStream)
    {
        const Token& tok = tokenStream.current();
        switch (tok.type)
        {
        case TokenType::NULL_LITERAL:
            tokenStream.advance();
            return TypedAnnotationValue::makeNull();
        case TokenType::INT_NUMBER:
        {
            int64_t v = tok.intValue;
            tokenStream.advance();
            return TypedAnnotationValue::makeInt(v);
        }
        case TokenType::FLOAT_NUMBER:
        {
            double v = tok.floatValue;
            tokenStream.advance();
            return TypedAnnotationValue::makeFloat(v);
        }
        case TokenType::TRUE:
            tokenStream.advance();
            return TypedAnnotationValue::makeBool(true);
        case TokenType::FALSE:
            tokenStream.advance();
            return TypedAnnotationValue::makeBool(false);
        case TokenType::STRING_LITERAL:
        {
            std::string v = tok.stringValue.getString();
            tokenStream.advance();
            return TypedAnnotationValue::makeString(std::move(v));
        }
        case TokenType::IDENTIFIER:
        {
            std::string v = tok.stringValue.getString();
            tokenStream.advance();
            return TypedAnnotationValue::makeClassRef(std::move(v));
        }
        case TokenType::LBRACKET:
            return parseArrayLiteral(tokenStream);
        default:
            throw ParseException(
                "Expected annotation literal value (int, float, bool, string, identifier, null, or [Class,...])",
                tok.location);
        }
    }

    TypedAnnotationValue AnnotationParser::parseArrayLiteral(TokenStream& tokenStream)
    {
        tokenStream.advance(); // consume '['
        std::vector<std::string> entries;
        while (tokenStream.current().type != TokenType::RBRACKET)
        {
            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected class identifier inside annotation array literal",
                                     tokenStream.current().location);
            }
            entries.push_back(tokenStream.current().stringValue.getString());
            tokenStream.advance();
            if (tokenStream.current().type == TokenType::COMMA)
            {
                tokenStream.advance();
                if (tokenStream.current().type == TokenType::RBRACKET)
                {
                    throw ParseException("Trailing ',' in annotation array literal",
                                         tokenStream.current().location);
                }
            }
            else if (tokenStream.current().type != TokenType::RBRACKET)
            {
                throw ParseException("Expected ',' or ']' in annotation array literal",
                                     tokenStream.current().location);
            }
        }
        tokenStream.advance(); // consume ']'
        return TypedAnnotationValue::makeClassArray(std::move(entries));
    }
}
