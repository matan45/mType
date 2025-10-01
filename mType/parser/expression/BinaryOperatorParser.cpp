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
            [this]() { return parseAdditive(); },
            {TokenType::LESS, TokenType::LESS_EQUALS, TokenType::GREATER, TokenType::GREATER_EQUALS}
        );
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
        catch (const std::exception&)
        {
            throw ParseException("Binary operator parsing failed", tokenStream.current().location);
        }
    }

    bool BinaryOperatorParser::isBinaryOperator(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::OR:
        case TokenType::AND:
        case TokenType::EQUALS:
        case TokenType::NOT_EQUALS:
        case TokenType::LESS:
        case TokenType::LESS_EQUALS:
        case TokenType::GREATER:
        case TokenType::GREATER_EQUALS:
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::MULTIPLY:
        case TokenType::DIVIDE:
        case TokenType::MODULO:
        case TokenType::QUESTION: // Ternary operator
            return true;
        default:
            return false;
        }
    }

    int BinaryOperatorParser::getOperatorPrecedence(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::OR:
            return 1;
        case TokenType::AND:
            return 2;
        case TokenType::EQUALS:
        case TokenType::NOT_EQUALS:
            return 3;
        case TokenType::LESS:
        case TokenType::LESS_EQUALS:
        case TokenType::GREATER:
        case TokenType::GREATER_EQUALS:
            return 4;
        case TokenType::PLUS:
        case TokenType::MINUS:
            return 5;
        case TokenType::MULTIPLY:
        case TokenType::DIVIDE:
        case TokenType::MODULO:
            return 6;
        case TokenType::QUESTION:
            return 0; // Lowest precedence
        default:
            return -1; // Not a binary operator
        }
    }
}
