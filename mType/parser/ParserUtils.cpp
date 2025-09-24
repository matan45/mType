#include "ParserUtils.hpp"
#include "TypeParser.hpp"
#include "TokenStream.hpp"
#include "../ast/ASTNode.hpp"
#include "../ast/GenericType.hpp"
#include "../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../errors/ParseException.hpp"
#include "../errors/SourceLocation.hpp"
#include <cctype>
#include <algorithm>

namespace parser
{
    std::string ParserUtils::buildQualifiedName(const std::vector<std::string>& parts) noexcept
    {
        if (parts.empty()) return {};
        if (parts.size() == 1) return parts[0];
        
        // Pre-calculate total size to avoid reallocations
        size_t totalSize = parts[0].size();
        for (size_t i = 1; i < parts.size(); ++i) {
            totalSize += 2 + parts[i].size(); // "::" + part
        }
        
        // Single allocation with exact size
        std::string result;
        result.reserve(totalSize);
        result = parts[0];
        
        // Efficient appending without temporary strings
        for (size_t i = 1; i < parts.size(); ++i) {
            result.append("::");
            result.append(parts[i]);
        }
        
        return result;
    }

    bool ParserUtils::isValidIdentifier(std::string_view name) noexcept
    {
        if (name.empty()) return false;
        
        // First character must be letter or underscore
        if (!std::isalpha(name[0]) && name[0] != '_') return false;
        
        // Remaining characters must be alphanumeric or underscore
        for (size_t i = 1; i < name.size(); ++i) {
            if (!std::isalnum(name[i]) && name[i] != '_') return false;
        }
        
        return true;
    }

    void ParserUtils::validateFunctionNamingConvention(std::string_view name, bool isStatic,
                                                      std::string_view context, const errors::SourceLocation& location)
    {
        using namespace errors;

        if (name.empty()) {
            throw ParseException(std::string(context) + " name cannot be empty", location);
        }

        // Check if first character is lowercase letter
        if (!std::islower(name[0]) && name[0] != '_') {
            std::string message = std::string(context) + " '" + std::string(name) +
                                "' must start with a lowercase letter. " +
                                (isStatic ? "Static methods" : "Functions and methods") +
                                " should follow camelCase convention.";
            throw ParseException(message, location);
        }

        // Additional validation: ensure it's a valid identifier
        if (!isValidIdentifier(name)) {
            throw ParseException(std::string(context) + " '" + std::string(name) + "' is not a valid identifier", location);
        }
    }

    std::vector<std::pair<std::string, value::ValueType>> ParserUtils::parseParameterList(TokenStream& stream, bool expectParentheses)
    {
        using namespace value;
        using namespace errors;
        using namespace token;
        
        std::vector<std::pair<std::string, ValueType>> parameters;

        if (expectParentheses) {
            stream.expect(TokenType::LPAREN);
        }

        // Handle empty parameter list
        if (stream.current().type == TokenType::RPAREN) {
            if (expectParentheses) {
                stream.advance(); // consume ')'
            }
            return parameters;
        }

        // Parse first parameter
        do {
            // Parse parameter type using centralized TypeParser
            ValueType paramType = TypeParser::parseType(stream);

            // Expect parameter name
            if (stream.current().type != TokenType::IDENTIFIER) {
                throw ParseException("Expected parameter name", stream.location());
            }

            std::string paramName = stream.current().stringValue.getString();
            stream.advance();

            // Add parameter to list
            parameters.emplace_back(paramName, paramType);

            // Check for more parameters
            if (stream.current().type == TokenType::COMMA) {
                stream.advance(); // consume ','
            } else {
                break; // End of parameter list
            }
        } while (stream.current().type != TokenType::RPAREN);

        if (expectParentheses) {
            stream.expect(TokenType::RPAREN);
        }

        return parameters;
    }

    std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>> ParserUtils::parseGenericParameterList(TokenStream& stream, bool expectParentheses)
    {
        using namespace value;
        using namespace errors;
        using namespace token;

        std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>> parameters;

        if (expectParentheses) {
            stream.expect(TokenType::LPAREN);
        }

        // Handle empty parameter list
        if (stream.current().type == TokenType::RPAREN) {
            if (expectParentheses) {
                stream.advance(); // consume ')'
            }
            return parameters;
        }

        // Parse first parameter
        do {
            // Parse parameter type using centralized TypeParser with generic support
            auto paramType = TypeParser::parseGenericType(stream);

            // Expect parameter name
            if (stream.current().type != TokenType::IDENTIFIER) {
                throw ParseException("Expected parameter name", stream.location());
            }

            std::string paramName = stream.current().stringValue.getString();
            stream.advance();

            // Add parameter to list
            parameters.emplace_back(paramName, paramType);

            // Check for more parameters
            if (stream.current().type == TokenType::COMMA) {
                stream.advance(); // consume ','
            } else {
                break; // End of parameter list
            }
        } while (stream.current().type != TokenType::RPAREN);

        if (expectParentheses) {
            stream.expect(TokenType::RPAREN);
        }

        return parameters;
    }

    std::unique_ptr<ast::ASTNode> ParserUtils::parseBinaryOperators(
        TokenStream& stream,
        std::function<std::unique_ptr<ast::ASTNode>()> parseNext,
        const std::vector<token::TokenType>& operators)
    {
        using namespace token;
        
        auto left = parseNext();
        
        while (std::find(operators.begin(), operators.end(), stream.current().type) != operators.end())
        {
            TokenType op = stream.current().type;
            stream.advance();
            auto right = parseNext();
            left = std::make_unique<ast::nodes::expressions::BinaryExpNode>(std::move(left), op, std::move(right));
        }
        
        return left;
    }
    
    token::TokenType ParserUtils::compoundToBinaryOperator(token::TokenType compoundOp)
    {
        using namespace token;
        using namespace errors;
        
        switch (compoundOp)
        {
        case TokenType::PLUS_ASSIGN: return TokenType::PLUS;
        case TokenType::MINUS_ASSIGN: return TokenType::MINUS;
        case TokenType::MULTIPLY_ASSIGN: return TokenType::MULTIPLY;
        case TokenType::DIVIDE_ASSIGN: return TokenType::DIVIDE;
        case TokenType::MODULO_ASSIGN: return TokenType::MODULO;
        default: 
            throw ParseException("Invalid compound assignment operator", SourceLocation());
        }
    }
    
    std::vector<std::string> ParserUtils::parseQualifiedIdentifierChain(
        TokenStream& stream, 
        std::string_view initialName)
    {
        using namespace token;
        using namespace errors;
        
        std::vector<std::string> parts = {std::string(initialName)};
        
        // We expect the stream to be positioned at the first identifier after the initial ::
        // Add that identifier to the parts
        parts.push_back(stream.current().stringValue.getString());
        stream.advance();
        
        // Continue parsing if there are more :: tokens
        while (stream.current().type == TokenType::SCOPE)
        {
            stream.advance(); // consume '::'
            
            if (stream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected identifier after '::'", stream.location());
            }
            
            parts.push_back(stream.current().stringValue.getString());
            stream.advance();
        }
        
        return parts;
    }
}