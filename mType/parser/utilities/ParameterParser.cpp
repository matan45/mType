#include "ParameterParser.hpp"
#include "NameValidator.hpp"
#include "../TypeParser.hpp"
#include "../TokenStream.hpp"
#include "../../ast/GenericType.hpp"
#include "../../errors/ParseException.hpp"
#include "../../errors/SourceLocation.hpp"
#include "../../token/TokenType.hpp"

namespace parser
{
    std::vector<std::pair<std::string, value::ValueType>> ParameterParser::parseParameterList(TokenStream& stream, bool expectParentheses)
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
            SourceLocation paramLocation = stream.location();

            // Validate parameter name contains no special characters
            NameValidator::validateIdentifierName(paramName, "Parameter", paramLocation);

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

    std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>> ParameterParser::parseGenericParameterList(TokenStream& stream, bool expectParentheses)
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
            SourceLocation paramLocation = stream.location();

            // Validate parameter name contains no special characters
            NameValidator::validateIdentifierName(paramName, "Parameter", paramLocation);

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

    std::vector<std::pair<std::string, value::ParameterType>> ParameterParser::parseParameterListWithTypes(TokenStream& stream, bool expectParentheses)
    {
        using namespace value;
        using namespace errors;
        using namespace token;

        std::vector<std::pair<std::string, ParameterType>> parameters;

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
            // Parse parameter type using TypeParser::parseTypeInfo to get interface information
            TypeInfo typeInfo = TypeParser::parseTypeInfo(stream);

            // Convert TypeInfo to ParameterType - single construction for efficiency
            ParameterType paramType =
                (typeInfo.baseType == ValueType::OBJECT && !typeInfo.className.empty())
                    ? ParameterType::forClass(typeInfo.className)
                    : ParameterType(typeInfo.baseType);
            paramType.nullable = typeInfo.isNullable;

            // Expect parameter name
            if (stream.current().type != TokenType::IDENTIFIER) {
                throw ParseException("Expected parameter name", stream.location());
            }

            std::string paramName = stream.current().stringValue.getString();
            SourceLocation paramLocation = stream.location();

            // Validate parameter name contains no special characters
            NameValidator::validateIdentifierName(paramName, "Parameter", paramLocation);

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
}
