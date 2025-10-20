#include "LiteralParser.hpp"
#include "ArgumentParser.hpp"
#include "../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../ast/nodes/expressions/FloatNode.hpp"
#include "../../ast/nodes/expressions/StringNode.hpp"
#include "../../ast/nodes/expressions/BoolNode.hpp"
#include "../../ast/nodes/expressions/NullNode.hpp"
#include "../../ast/nodes/expressions/VariableNode.hpp"
#include "../../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../ast/nodes/classes/SuperConstructorCallNode.hpp"
#include "../../ast/nodes/classes/SuperMethodCallNode.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::expression
{
    using namespace ast::nodes::expressions;
    using namespace token;
    using namespace errors;

    LiteralParser::LiteralParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }

    std::unique_ptr<ASTNode> LiteralParser::parse()
    {
        return parsePrimary();
    }

    bool LiteralParser::canParse(const TokenStream& stream) const
    {
        return isLiteralToken(stream.current().type);
    }

    std::unique_ptr<ASTNode> LiteralParser::parsePrimary()
    {
        switch (tokenStream.current().type)
        {
        case TokenType::INT_NUMBER:
            return parseIntegerLiteral();
        case TokenType::FLOAT_NUMBER:
            return parseFloatLiteral();
        case TokenType::STRING_LITERAL:
            return parseStringLiteral();
        case TokenType::TRUE:
        case TokenType::FALSE:
            return parseBooleanLiteral();
        case TokenType::NULL_LITERAL:
            return parseNullLiteral();
        case TokenType::IDENTIFIER:
            return parseIdentifier();
        case TokenType::LPAREN:
            return parseParenthesizedExpression();
        case TokenType::NEW:
            // Delegate to ClassParser for proper separation of concerns
            return context.parseNewExpression();
        case TokenType::LBRACKET:
            // Parse array literal: [1, 2, 3]
            return parseArrayLiteral();
        case TokenType::SUPER:
            return parseSuperExpression();
        default:
            throw ParseException("Unexpected token in primary expression", tokenStream.current().location);
        }
    }

    std::unique_ptr<ASTNode> LiteralParser::parseArrayLiteral()
    {
        // Parse array literal: [element1, element2, element3, ...]
        SourceLocation location = tokenStream.current().location;
        expectToken(TokenType::LBRACKET);

        std::vector<std::unique_ptr<ASTNode>> elements;

        // Handle empty array: []
        if (tokenStream.check(TokenType::RBRACKET))
        {
            tokenStream.advance(); // consume ']'
            return std::make_unique<ArrayLiteralNode>(std::move(elements), location);
        }

        // Parse first element
        elements.push_back(context.parseExpression());

        // Parse additional elements separated by commas
        while (tryConsumeToken(TokenType::COMMA))
        {
            elements.push_back(context.parseExpression());
        }

        expectToken(TokenType::RBRACKET);
        return std::make_unique<ArrayLiteralNode>(std::move(elements), location);
    }

    std::unique_ptr<ASTNode> LiteralParser::parseIntegerLiteral()
    {
        int value = tokenStream.current().intValue;
        auto intLocation = tokenStream.current().location;
        tokenStream.advance();
        return std::make_unique<IntegerNode>(value, intLocation);
    }

    std::unique_ptr<ASTNode> LiteralParser::parseFloatLiteral()
    {
        float value = tokenStream.current().floatValue;
        auto floatLocation = tokenStream.current().location;
        tokenStream.advance();
        return std::make_unique<FloatNode>(value, floatLocation);
    }

    std::unique_ptr<ASTNode> LiteralParser::parseStringLiteral()
    {
        std::string value = tokenStream.current().stringValue.getString();
        auto stringLocation = tokenStream.current().location;
        tokenStream.advance();
        return std::make_unique<StringNode>(value, stringLocation);
    }

    std::unique_ptr<ASTNode> LiteralParser::parseBooleanLiteral()
    {
        bool value = (tokenStream.current().type == TokenType::TRUE);
        auto boolLocation = tokenStream.current().location;
        tokenStream.advance();
        return std::make_unique<BoolNode>(value, boolLocation);
    }

    std::unique_ptr<ASTNode> LiteralParser::parseNullLiteral()
    {
        auto nullLocation = tokenStream.current().location;
        tokenStream.advance();
        return std::make_unique<NullNode>(nullLocation);
    }

    std::unique_ptr<ASTNode> LiteralParser::parseIdentifier()
    {
        std::string name = tokenStream.current().stringValue.getString();
        auto identifierLocation = tokenStream.current().location;
        tokenStream.advance();
        return std::make_unique<VariableNode>(name, identifierLocation);
    }

    std::unique_ptr<ASTNode> LiteralParser::parseParenthesizedExpression()
    {
        expectToken(TokenType::LPAREN);
        auto expr = context.parseExpression(); // Delegate back to main expression parsing
        expectToken(TokenType::RPAREN);
        return expr;
    }

    std::unique_ptr<ASTNode> LiteralParser::parseSuperExpression()
    {
        using namespace ast::nodes::classes;

        SourceLocation superLocation = tokenStream.current().location;
        expectToken(TokenType::SUPER);

        // Check what follows 'super'
        if (tokenStream.check(TokenType::LPAREN))
        {
            // super(...) - constructor call

            // Validate: super() constructor calls are not allowed inside constructor bodies
            // They must use the initializer list syntax: constructor(...) : super(...) { }
            if (context.isInsideConstructorBody())
            {
                throw ParseException(
                    "super() constructor calls are not allowed inside constructor bodies. "
                    "Use the constructor initializer list syntax instead: constructor(...) : super(...) { }",
                    superLocation);
            }

            ArgumentParser argParser(tokenStream, context);
            std::vector<std::unique_ptr<ASTNode>> arguments = argParser.parseArgumentsWithParentheses();

            return std::make_unique<SuperConstructorCallNode>(
                std::move(arguments), superLocation);
        }
        else if (tokenStream.check(TokenType::DOT))
        {
            // super.method(...) - method call
            tokenStream.advance(); // consume '.'

            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected method name after 'super.'",
                                   tokenStream.current().location);
            }

            std::string methodName = tokenStream.current().stringValue.getString();
            tokenStream.advance();

            // Expect method call with parentheses
            ArgumentParser argParser(tokenStream, context);
            std::vector<std::unique_ptr<ASTNode>> arguments = argParser.parseArgumentsWithParentheses();

            return std::make_unique<SuperMethodCallNode>(
                methodName, std::move(arguments), superLocation);
        }
        else
        {
            throw ParseException("Expected '(' or '.' after 'super'",
                               tokenStream.current().location);
        }
    }

    bool LiteralParser::isLiteralToken(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::INT_NUMBER:
        case TokenType::FLOAT_NUMBER:
        case TokenType::STRING_LITERAL:
        case TokenType::TRUE:
        case TokenType::FALSE:
        case TokenType::NULL_LITERAL:
        case TokenType::IDENTIFIER:
        case TokenType::LPAREN:
        case TokenType::NEW:
        case TokenType::LBRACKET:
        case TokenType::SUPER:
            return true;
        default:
            return false;
        }
    }
}
