#include "BinaryOperatorParser.hpp"
#include "../ExpressionParser.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::expression
{
    using namespace ast::nodes::expressions;
    using namespace token;
    using namespace errors;

    BinaryOperatorParser::BinaryOperatorParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx), expressionParser(nullptr)
    {
    }

    void BinaryOperatorParser::setExpressionParser(ExpressionParser& exprParser)
    {
        expressionParser = &exprParser;
    }

    std::unique_ptr<ASTNode> BinaryOperatorParser::parse()
    {
        return parseTernary();
    }

    bool BinaryOperatorParser::canParse(const TokenStream& stream) const
    {
        // Binary operators can parse any expression that starts with unary or primary
        return true; // This will be called from higher level parsers
    }

    std::unique_ptr<ASTNode> BinaryOperatorParser::parseTernary()
    {
        auto expr = parseLogicalOr();

        if (tokenStream.check(TokenType::QUESTION))
        {
            SourceLocation questionLocation = tokenStream.current().location; // Capture location before advancing
            tokenStream.advance();
            auto trueExpr = parseTernary(); // Use same level to avoid recursion
            expectToken(TokenType::COLON);
            auto falseExpr = parseTernary(); // Use same level to avoid recursion
            return std::make_unique<TernaryExpNode>(std::move(expr), std::move(trueExpr), std::move(falseExpr),
                                                    questionLocation);
        }

        return expr;
    }

    std::unique_ptr<ASTNode> BinaryOperatorParser::parseLogicalOr()
    {
        return parseBinaryLevel(
            [this]() { return parseLogicalAnd(); },
            {TokenType::OR}
        );
    }

    std::unique_ptr<ASTNode> BinaryOperatorParser::parseLogicalAnd()
    {
        return parseBinaryLevel(
            [this]() { return parseEquality(); },
            {TokenType::AND}
        );
    }

    std::unique_ptr<ASTNode> BinaryOperatorParser::parseEquality()
    {
        return parseBinaryLevel(
            [this]() { return parseComparison(); },
            {TokenType::EQUALS, TokenType::NOT_EQUALS}
        );
    }

    std::unique_ptr<ASTNode> BinaryOperatorParser::parseComparison()
    {
        return parseBinaryLevel(
            [this]() { return parseIsClassOf(); },
            {TokenType::LESS, TokenType::LESS_EQUALS, TokenType::GREATER, TokenType::GREATER_EQUALS}
        );
    }

    std::unique_ptr<ASTNode> BinaryOperatorParser::parseIsClassOf()
    {
        auto left = parseAdditive();

        // Check for isClassOf operator
        while (tokenStream.check(TokenType::ISCLASSOF))
        {
            // Delegate to castParser for instanceof handling
            if (expressionParser)
            {
                left = expressionParser->getCastParser()->parseInstanceOfExpression(std::move(left));
            }
            else
            {
                throw ParseException("ExpressionParser not initialized in BinaryOperatorParser",
                                     tokenStream.current().location);
            }
        }

        return left;
    }

    std::unique_ptr<ASTNode> BinaryOperatorParser::parseAdditive()
    {
        return parseBinaryLevel(
            [this]() { return parseMultiplicative(); },
            {TokenType::PLUS, TokenType::MINUS}
        );
    }

    std::unique_ptr<ASTNode> BinaryOperatorParser::parseMultiplicative()
    {
        return parseBinaryLevel(
            [this]()
            {
                if (!expressionParser)
                {
                    throw ParseException("ExpressionParser not initialized in BinaryOperatorParser",
                                         tokenStream.current().location);
                }
                return expressionParser->parseUnary();
            },
            {TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::MODULO}
        );
    }

    std::unique_ptr<ASTNode> BinaryOperatorParser::parseBinaryLevel(
        std::function<std::unique_ptr<ASTNode>()> parseNext,
        const std::vector<TokenType>& operators)
    {
        try
        {
            return ParserUtils::parseBinaryOperators(tokenStream, parseNext, operators);
        }
        catch (const ParseException&)
        {
            // Re-throw parse exceptions to preserve error messages
            throw;
        }
        catch (const std::exception& e)
        {
            throw ParseException("Binary operator parsing failed: " + std::string(e.what()),
                                 tokenStream.current().location);
        }
    }
}
