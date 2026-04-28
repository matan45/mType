#include "CastParser.hpp"
#include "../ExpressionParser.hpp"
#include "../TypeParser.hpp"
#include "../../ast/nodes/expressions/CastExpression.hpp"
#include "../../ast/nodes/expressions/InstanceOfExpression.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::expression
{
    using namespace ast::nodes::expressions;
    using namespace errors;

    CastParser::CastParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx), expressionParser(nullptr)
    {
    }

    void CastParser::setExpressionParser(ExpressionParser& exprParser)
    {
        expressionParser = &exprParser;
    }

    std::unique_ptr<ASTNode> CastParser::parse()
    {
        if (canParse(tokenStream))
        {
            return parseCastExpression();
        }
        return nullptr;
    }

    bool CastParser::canParse(const TokenStream& stream) const
    {
        if (!stream.check(TokenType::LPAREN))
        {
            return false;
        }

        try
        {
            return isCastExpression();
        }
        catch (...)
        {
            // If any exception occurs during lookahead (e.g., bracket balancer errors),
            // assume it's not a cast expression
            return false;
        }
    }

    bool CastParser::isCastExpression() const
    {
        // Lookahead to distinguish (Type)expr from (expr)
        // Check token after LPAREN without advancing
        Token nextToken = tokenStream.peek();

        // Check if it's a type name
        TokenType type = nextToken.type;

        // Check for primitive types (including primitive arrays)
        if (type == TokenType::INT || type == TokenType::FLOAT ||
            type == TokenType::BOOL || type == TokenType::STRING_TYPE ||
            type == TokenType::VOID)
        {
            return true; // Handles: (int), (float[]), (string[][]), etc.
        }

        // Check for identifier (class/interface name)
        if (type == TokenType::IDENTIFIER)
        {
            // Additional lookahead: if followed by ), it's likely a cast
            // We're at '(', peek() gave us the identifier, so peekAhead(2) gives us what's after the identifier
            Token afterId = tokenStream.peekAhead(2); // Token after identifier

            if (afterId.type == TokenType::RPAREN)
            {
                return true; // Pattern: (Type)
            }

            // Check for array types: (Type[]), (Type[][]), etc.
            if (afterId.type == TokenType::LBRACKET)
            {
                std::string idValue = std::string(nextToken.stringValue);
                // Check if first character is uppercase (type convention)
                if (!idValue.empty() && std::isupper(idValue[0]))
                {
                    return true; // Likely an array type cast
                }
            }

            // For generic types like (Circle<T>), we need more sophisticated checking
            // Only treat it as a cast if the identifier starts with uppercase
            // (by convention, types start with uppercase, variables with lowercase)
            if (afterId.type == TokenType::LESS)
            {
                std::string idValue = std::string(nextToken.stringValue);
                // Check if first character is uppercase (type convention)
                if (!idValue.empty() && std::isupper(idValue[0]))
                {
                    return true; // Likely a generic type cast
                }
            }

            // Pattern: (Type?) — nullable cast attempt. Disambiguator vs ternary
            // "(x ? a : b)": require ')' immediately after '?'. We commit to the
            // cast path here so parseCastExpression can emit a targeted error.
            if (afterId.type == TokenType::QUESTION)
            {
                Token afterQuestion = tokenStream.peekAhead(3);
                if (afterQuestion.type == TokenType::RPAREN)
                {
                    std::string idValue = std::string(nextToken.stringValue);
                    if (!idValue.empty() && std::isupper(static_cast<unsigned char>(idValue[0])))
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }


    std::unique_ptr<ASTNode> CastParser::parseCastExpression()
    {
        SourceLocation startLoc = tokenStream.current().location;

        expectToken(TokenType::LPAREN);

        // Parse target type using TypeParser
        SourceLocation typeStart = tokenStream.current().location;
        auto targetType = TypeParser::parseGenericType(tokenStream);

        // Reject (T?) cast syntax — assigning null is expressed via T? variables.
        // Catches (T?), (T[]?), (T<X>?), (T<X>[]?) uniformly.
        if (targetType->isNullable())
        {
            throw ParseException(
                "Nullable cast syntax (T?) is not supported. "
                "Cast to T and assign to a T? variable, or use null directly.",
                typeStart);
        }

        expectToken(TokenType::RPAREN);

        // Parse expression to cast
        if (!expressionParser)
        {
            throw ParseException("ExpressionParser not initialized in CastParser",
                                 tokenStream.current().location);
        }

        // Parse the highest precedence expression (unary or higher)
        auto expr = expressionParser->parseUnary();

        return std::make_unique<CastExpression>(
            std::move(expr),
            std::move(targetType),
            startLoc
        );
    }

    std::unique_ptr<ASTNode> CastParser::parseInstanceOfExpression(std::unique_ptr<ASTNode> left)
    {
        SourceLocation loc = tokenStream.current().location;

        // Consume isClassOf token
        if (!tokenStream.check(TokenType::ISCLASSOF))
        {
            throw ParseException("Expected 'isClassOf'",
                                 tokenStream.current().location);
        }
        tokenStream.advance();

        // Parse target type using TypeParser
        auto targetType = TypeParser::parseGenericType(tokenStream);

        return std::make_unique<InstanceOfExpression>(
            std::move(left),
            std::move(targetType),
            loc
        );
    }
}
