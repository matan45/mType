#include "UnaryOperatorParser.hpp"
#include "../ExpressionParser.hpp"
#include "../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../exceptions/DomainExceptions.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::expression
{
    using namespace ast::nodes::expressions;
    using namespace token;
    using namespace errors;

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
        // Handle prefix unary operators like !, -, +, ++, --
        if (isPrefixUnaryOperator(tokenStream.current().type))
        {
            TokenType op = tokenStream.current().type;
            tokenStream.advance();
            auto operand = parseUnary(); // Support chained unary operators
            return std::make_unique<UnaryExpNode>(op, std::move(operand), UnaryPosition::PREFIX);
        }

        // If not a unary operator, delegate to postfix parsing
        if (!expressionParser)
        {
            reportError("ExpressionParser not set in UnaryOperatorParser", getParserName());
            throw errors::ParseException("ExpressionParser not initialized in UnaryOperatorParser");
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