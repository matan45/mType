#include "UnaryOperatorParser.hpp"
#include "../ExpressionParser.hpp"
#include "../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::expression
{
    using namespace ast::nodes::expressions;
    using namespace token;
    using namespace errors;

    UnaryOperatorParser::UnaryOperatorParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx), expressionParser(nullptr)
    {
    }

    void UnaryOperatorParser::setExpressionParser(ExpressionParser& exprParser)
    {
        expressionParser = &exprParser;
    }


    std::unique_ptr<ASTNode> UnaryOperatorParser::parse()
    {
        return parseUnary();
    }

    bool UnaryOperatorParser::canParse(const TokenStream& stream) const
    {
        return isPrefixUnaryOperator(stream.current().type);
    }

    std::unique_ptr<ASTNode> UnaryOperatorParser::parseUnary()
    {
        // Check for cast expression first: (Type)expression
        if (expressionParser && expressionParser->getCastParser()->isCastExpression())
        {
            return expressionParser->getCastParser()->parseCastExpression();
        }

        // Handle prefix unary operators like !, -, +, ++, --
        if (isPrefixUnaryOperator(tokenStream.current().type))
        {
            TokenType op = tokenStream.current().type;
            SourceLocation opLocation = tokenStream.current().location; // Capture location before advancing
            tokenStream.advance();
            auto operand = parseUnary(); // Support chained unary operators
            return std::make_unique<UnaryExpNode>(op, std::move(operand), UnaryPosition::PREFIX, opLocation);
        }

        // If not a unary operator, delegate to postfix parsing
        if (!expressionParser)
        {
            throw ParseException("ExpressionParser not initialized in UnaryOperatorParser",
                                 tokenStream.current().location);
        }
        return expressionParser->parsePostfix();
    }

    bool UnaryOperatorParser::isPrefixUnaryOperator(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::NOT:
        case TokenType::MINUS:
        case TokenType::PLUS:
        case TokenType::INCREMENT:
        case TokenType::DECREMENT:
            return true;
        default:
            return false;
        }
    }
}
