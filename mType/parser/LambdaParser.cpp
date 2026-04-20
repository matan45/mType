#include "LambdaParser.hpp"
#include "ExpressionParser.hpp"
#include "../token/TokenType.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    LambdaParser::LambdaParser(TokenStream& stream, ParseContext& ctx)
        : tokenStream(stream), context(ctx)
    {
    }

    std::unique_ptr<LambdaNode> LambdaParser::parseLambda()
    {
        if (!isLambdaStart())
        {
            // Debug: log current token info
            std::string tokenInfo = "Current token: type=" + std::to_string(
                static_cast<int>(tokenStream.current().type));
            if (tokenStream.current().type == TokenType::IDENTIFIER)
            {
                tokenInfo += ", value=";
                tokenInfo += tokenStream.current().stringValue;
            }
            if (!tokenStream.isAtEnd())
            {
                tokenInfo += ", next type=" + std::to_string(static_cast<int>(tokenStream.peek().type));
            }
            throw ParseException("Expected lambda expression. " + tokenInfo, tokenStream.current().location);
        }

        auto location = tokenStream.current().location;

        // Parse parameters
        std::vector<Parameter> parameters = parseLambdaParameters();

        // Expect arrow
        if (tokenStream.current().type != TokenType::ARROW)
        {
            throw ParseException("Expected '->' in lambda expression", tokenStream.current().location);
        }
        tokenStream.advance(); // consume '->'

        // Parse body with lambda context guard
        auto [body, bodyType] = parseLambdaBody();

        return std::make_unique<LambdaNode>(parameters, std::move(body), location, bodyType);
    }

    bool LambdaParser::isLambdaStart() const
    {
        // Pattern 1: identifier -> (single parameter without parentheses)
        if (tokenStream.current().type == TokenType::IDENTIFIER)
        {
            if (!tokenStream.isAtEnd() && tokenStream.peek().type == TokenType::ARROW)
            {
                return true;
            }
        }

        // Pattern 2: ( ... ) -> (parenthesized parameters)
        if (tokenStream.current().type == TokenType::LPAREN)
        {
            return hasArrowAhead();
        }

        return false;
    }

    std::vector<Parameter> LambdaParser::parseLambdaParameters()
    {
        std::vector<Parameter> parameters;

        // Parse lambda parameters

        if (tokenStream.current().type == TokenType::IDENTIFIER &&
            !tokenStream.isAtEnd() && tokenStream.peek().type == TokenType::ARROW)
        {
            // Single parameter without parentheses: param ->
            std::string paramName = std::string(tokenStream.current().stringValue);
            tokenStream.advance();
            parameters.emplace_back(paramName);
        }
        else if (tokenStream.match(TokenType::LPAREN))
        {
            // Parenthesized parameter list: () or (params)

            // Check for empty parameter list: ()
            if (tokenStream.check(TokenType::RPAREN))
            {
                tokenStream.advance(); // consume ')'
                return parameters; // empty parameter list
            }

            // Parse comma-separated parameter list
            do
            {
                parameters.push_back(parseParameter());
            }
            while (tokenStream.match(TokenType::COMMA));

            // Expect closing parenthesis
            tokenStream.expect(TokenType::RPAREN);
        }
        else
        {
            throw ParseException("Expected parameter list or identifier before '->'", tokenStream.current().location);
        }

        return parameters;
    }

    Parameter LambdaParser::parseParameter()
    {
        // Expect parameter name
        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected parameter name", tokenStream.current().location);
        }

        std::string paramName = std::string(tokenStream.current().stringValue);
        tokenStream.advance();

        std::shared_ptr<ast::GenericType> paramType = nullptr;

        // Optional type annotation: param : Type
        if (tokenStream.match(TokenType::COLON))
        {
            // Parse type name
            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected type name after ':'", tokenStream.current().location);
            }

            std::string typeName = std::string(tokenStream.current().stringValue);
            tokenStream.advance();

            // Create a simple type - this should be enhanced later for complex types
            paramType = std::make_shared<ast::GenericType>(typeName);
        }

        return Parameter(paramName, paramType);
    }

    std::pair<std::unique_ptr<ASTNode>, BodyType> LambdaParser::parseLambdaBody()
    {
        if (tokenStream.current().type == TokenType::LBRACE)
        {
            // Block lambda: { statements }
            auto blockNode = context.parseStatement();
            return {std::move(blockNode), BodyType::BLOCK};
        }
        // Expression lambda: parse expression until natural boundary
        // The expression should stop at terminators like ';', ')', '}', ','
        // Use lower precedence parsing to avoid consuming assignment operators
        ExpressionParser exprParser(tokenStream, context);
        auto exprNode = exprParser.parseTernary(); // Skip assignment level to avoid conflicts
        return {std::move(exprNode), BodyType::EXPRESSION};
    }

    bool LambdaParser::hasArrowAhead() const
    {
        // For now, we'll use a simpler heuristic
        // Check for empty parameter list: () ->
        if (tokenStream.current().type == TokenType::LPAREN)
        {
            Token next = tokenStream.peek();
            if (next.type == TokenType::RPAREN)
            {
                // This is likely () -> pattern
                return true;
            }
            // For more complex parameter lists, we'll be more conservative
            // and let the parser attempt to parse and handle errors
            return true; // Assume it might be a lambda for now
        }

        return false;
    }
}
