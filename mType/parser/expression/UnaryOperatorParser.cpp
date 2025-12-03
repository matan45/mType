#include "UnaryOperatorParser.hpp"
#include "../ExpressionParser.hpp"
#include "../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../ast/nodes/expressions/AwaitExpression.hpp"
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
        if (expressionParser && expressionParser->getCastParser()->canParse(tokenStream))
        {
            return expressionParser->getCastParser()->parseCastExpression();
        }

        // NEW: Handle await expression
        if (tokenStream.current().type == TokenType::AWAIT)
        {
            SourceLocation awaitLocation = tokenStream.current().location;
            tokenStream.advance(); // consume 'await'

            // Disallow await in constructors (constructors cannot be async)
            if (context.isInsideConstructorBody())
            {
                throw ParseException("'await' cannot be used inside constructors (constructors cannot be async)",
                                     awaitLocation);
            }

            // Allow await in:
            // 1. Async functions (context.isInsideAsyncFunction())
            // 2. Top-level/global scope (not inside any function)
            // Disallow await in non-async functions
            bool isInNonAsyncFunction = context.isInsideFunctionBody() && !context.isInsideAsyncFunction();

            if (isInNonAsyncFunction)
            {
                throw ParseException("'await' can only be used inside async functions or at the top level",
                                     awaitLocation);
            }

            // Parse the expression being awaited
            auto expression = parseUnary(); // Support await await foo() if needed

            return std::make_unique<AwaitExpression>(std::move(expression), awaitLocation);
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
        case TokenType::BITWISE_NOT:
            return true;
        default:
            return false;
        }
    }
}
