#include "AnnotationParser.hpp"
#include <cstdint>
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

        std::string annotationName = std::string(tokenStream.current().stringValue);
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
        //   IDENT = literal             -> named argument
        //   IDENT , IDENT [, ...]       -> legacy bare-identifier list (@Throw(IO,Net))
        //   anything else (incl. lone   -> positional shorthand (single literal)
        //   IDENT before RPAREN)
        //
        // The legacy form requires AT LEAST two identifiers separated by a
        // comma — a lone `IDENT RPAREN` would otherwise collide with the
        // single-arg positional form (`@Retention(SOURCE)`, `@Throw(IOExc)`),
        // which is a more natural spelling for one-shot Class-ref
        // annotations. A single bare IDENT still reaches @Throw via the
        // CLASS_REF → CLASS_ARRAY widening in the usage validator.
        bool firstIsIdent  = (tokenStream.current().type == TokenType::IDENTIFIER);
        bool secondIsAssign =
            firstIsIdent && (tokenStream.peek().type == TokenType::ASSIGN);
        bool legacyIdentList =
            firstIsIdent && !secondIsAssign &&
            tokenStream.peek().type == TokenType::COMMA;

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
                classNames.push_back(std::string(tokenStream.current().stringValue));
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
                std::string key = std::string(tokenStream.current().stringValue);
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
            std::string v = std::string(tok.stringValue);
            tokenStream.advance();
            return TypedAnnotationValue::makeString(std::move(v));
        }
        case TokenType::IDENTIFIER:
        {
            std::string v = std::string(tok.stringValue);
            tokenStream.advance();
            return TypedAnnotationValue::makeClassRef(std::move(v));
        }
        case TokenType::LBRACKET:
            return parseArrayLiteral(tokenStream);
        default:
            throw ParseException(
                "Expected annotation literal value (int, float, bool, string, identifier, null, or supported array literal)",
                tok.location);
        }
    }

    TypedAnnotationValue AnnotationParser::parseArrayLiteral(TokenStream& tokenStream)
    {
        tokenStream.advance(); // consume '['

        if (tokenStream.current().type == TokenType::RBRACKET)
        {
            tokenStream.advance();
            return TypedAnnotationValue::makeClassArray({});
        }

        const TokenType firstType = tokenStream.current().type;
        if (firstType == TokenType::IDENTIFIER)
        {
            std::vector<std::string> entries;
            while (tokenStream.current().type != TokenType::RBRACKET)
            {
                if (tokenStream.current().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected class identifier inside annotation Class[] literal",
                                         tokenStream.current().location);
                }
                entries.push_back(std::string(tokenStream.current().stringValue));
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
            tokenStream.advance();
            return TypedAnnotationValue::makeClassArray(std::move(entries));
        }

        if (firstType == TokenType::STRING_LITERAL)
        {
            std::vector<std::string> entries;
            while (tokenStream.current().type != TokenType::RBRACKET)
            {
                if (tokenStream.current().type != TokenType::STRING_LITERAL)
                {
                    throw ParseException("Expected string literal inside annotation string[] literal",
                                         tokenStream.current().location);
                }
                entries.push_back(std::string(tokenStream.current().stringValue));
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
            tokenStream.advance();
            return TypedAnnotationValue::makeStringArray(std::move(entries));
        }

        if (firstType == TokenType::TRUE || firstType == TokenType::FALSE)
        {
            std::vector<bool> entries;
            while (tokenStream.current().type != TokenType::RBRACKET)
            {
                if (tokenStream.current().type == TokenType::TRUE) entries.push_back(true);
                else if (tokenStream.current().type == TokenType::FALSE) entries.push_back(false);
                else
                {
                    throw ParseException("Expected boolean literal inside annotation bool[] literal",
                                         tokenStream.current().location);
                }

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
            tokenStream.advance();
            return TypedAnnotationValue::makeBoolArray(std::move(entries));
        }

        if (firstType == TokenType::INT_NUMBER || firstType == TokenType::FLOAT_NUMBER)
        {
            std::vector<int64_t> ints;
            std::vector<double> floats;
            bool hasFloat = false;

            while (tokenStream.current().type != TokenType::RBRACKET)
            {
                if (tokenStream.current().type == TokenType::INT_NUMBER)
                {
                    ints.push_back(tokenStream.current().intValue);
                    floats.push_back(static_cast<double>(tokenStream.current().intValue));
                }
                else if (tokenStream.current().type == TokenType::FLOAT_NUMBER)
                {
                    hasFloat = true;
                    floats.push_back(tokenStream.current().floatValue);
                }
                else
                {
                    throw ParseException("Expected numeric literal inside annotation numeric[] literal",
                                         tokenStream.current().location);
                }

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
            tokenStream.advance();
            if (hasFloat) return TypedAnnotationValue::makeFloatArray(std::move(floats));
            return TypedAnnotationValue::makeIntArray(std::move(ints));
        }

        throw ParseException("Unsupported annotation array literal element type",
                             tokenStream.current().location);
    }
}
