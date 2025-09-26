#include "LambdaParser.hpp"
#include "../token/TokenType.hpp"
#include "../errors/ParseException.hpp"
#include <stdexcept>

namespace parser
{
    std::unique_ptr<LambdaNode> LambdaParser::parseLambda()
    {
        if (!isLambdaStart()) {
            throw ParseException("Expected lambda expression", tokenStream.current().location);
        }

        auto location = tokenStream.current().location;

        // Parse parameters
        std::vector<Parameter> parameters = parseLambdaParameters();

        // Expect arrow
        if (tokenStream.current().type != TokenType::ARROW) {
            throw ParseException("Expected '->' in lambda expression", tokenStream.current().location);
        }
        tokenStream.advance(); // consume '->'

        // Parse body
        auto [body, bodyType] = parseLambdaBody();

        return std::make_unique<LambdaNode>(parameters, std::move(body), location, bodyType);
    }

    bool LambdaParser::isLambdaStart() const
    {
        // Pattern 1: identifier -> (single parameter without parentheses)
        if (tokenStream.current().type == TokenType::IDENTIFIER) {
            if (!tokenStream.isAtEnd() && tokenStream.peek().type == TokenType::ARROW) {
                return true;
            }
        }

        // Pattern 2: ( ... ) -> (parenthesized parameters)
        if (tokenStream.current().type == TokenType::LPAREN) {
            return hasArrowAhead();
        }

        return false;
    }

    std::vector<Parameter> LambdaParser::parseLambdaParameters()
    {
        std::vector<Parameter> parameters;

        if (tokenStream.current().type == TokenType::IDENTIFIER &&
            !tokenStream.isAtEnd() && tokenStream.peek().type == TokenType::ARROW) {
            // Single parameter without parentheses: param ->
            std::string paramName = tokenStream.current().stringValue.getString();
            tokenStream.advance();
            parameters.emplace_back(paramName);
        }
        else if (tokenStream.current().type == TokenType::LPAREN) {
            // Parenthesized parameter list: () or (params)
            tokenStream.advance(); // consume '('

            if (tokenStream.current().type != TokenType::RPAREN) {
                // Parse parameter list
                do {
                    if (tokenStream.current().type != TokenType::IDENTIFIER) {
                        throw ParseException("Expected parameter name", tokenStream.current().location);
                    }

                    std::string paramName = tokenStream.current().stringValue.getString();
                    tokenStream.advance();

                    std::shared_ptr<ast::GenericType> paramType = nullptr;

                    // Optional type annotation: param : Type
                    if (tokenStream.current().type == TokenType::COLON) {
                        tokenStream.advance(); // consume ':'
                        // For now, we'll handle basic type parsing
                        // This should be enhanced to use a proper type parser
                        if (tokenStream.current().type == TokenType::IDENTIFIER) {
                            std::string typeName = tokenStream.current().stringValue.getString();
                            tokenStream.advance();
                            // Create a simple type - this should be enhanced later
                            paramType = std::make_shared<ast::GenericType>(typeName);
                        } else {
                            throw ParseException("Expected type name after ':'", tokenStream.current().location);
                        }
                    }

                    parameters.emplace_back(paramName, paramType);

                } while (tokenStream.current().type == TokenType::COMMA &&
                        (tokenStream.advance(), true)); // consume comma and continue
            }

            if (tokenStream.current().type != TokenType::RPAREN) {
                throw ParseException("Expected ')' after parameter list", tokenStream.current().location);
            }
            tokenStream.advance(); // consume ')'
        }
        else {
            throw ParseException("Expected parameter list or identifier before '->'", tokenStream.current().location);
        }

        return parameters;
    }

    std::pair<std::unique_ptr<ASTNode>, BodyType> LambdaParser::parseLambdaBody()
    {
        if (tokenStream.current().type == TokenType::LBRACE) {
            // Block lambda: { statements }
            auto blockNode = context.parseStatement();
            return {std::move(blockNode), BodyType::BLOCK};
        } else {
            // Expression lambda: expression
            auto exprNode = context.parseExpression();
            return {std::move(exprNode), BodyType::EXPRESSION};
        }
    }

    bool LambdaParser::hasArrowAhead() const
    {
        // For now, we'll use a simpler heuristic
        // Check for empty parameter list: () ->
        if (tokenStream.current().type == TokenType::LPAREN) {
            Token next = tokenStream.peek();
            if (next.type == TokenType::RPAREN) {
                // This is likely () -> pattern
                return true;
            }
            // For more complex parameter lists, we'll be more conservative
            // and let the parser attempt to parse and handle errors
            return true; // Assume it might be a lambda for now
        }

        return false;
    }

    bool LambdaParser::isLambdaParameterPattern() const
    {
        // Check for valid lambda parameter patterns
        TokenType current = tokenStream.current().type;

        // Empty parameter list: ()
        if (current == TokenType::LPAREN) {
            if (!tokenStream.isAtEnd() && tokenStream.peek().type == TokenType::RPAREN) {
                return true;
            }
        }

        // Identifier (could be single parameter or start of parameter list)
        if (current == TokenType::IDENTIFIER) {
            return true;
        }

        return false;
    }

    int LambdaParser::findMatchingParen() const
    {
        // Simplified version - we'll handle paren matching differently
        // This method is not currently used but kept for potential future use
        return -1;
    }
}
