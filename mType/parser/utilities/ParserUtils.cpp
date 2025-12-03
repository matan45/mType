#include "ParserUtils.hpp"
#include "ParameterParser.hpp"
#include "NameValidator.hpp"
#include "QualifiedNameParser.hpp"
#include "../TokenStream.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../ast/GenericType.hpp"
#include "../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../errors/ParseException.hpp"
#include "../../errors/SourceLocation.hpp"
#include "../../token/TokenType.hpp"
#include <algorithm>

namespace parser
{
    // ============================================================================
    // LEGACY FORWARDING METHODS - For backward compatibility
    // ============================================================================

    std::string ParserUtils::buildQualifiedName(const std::vector<std::string>& parts) noexcept
    {
        return QualifiedNameParser::buildQualifiedName(parts);
    }

    bool ParserUtils::isValidIdentifier(std::string_view name) noexcept
    {
        return NameValidator::isValidIdentifier(name);
    }

    void ParserUtils::validateFunctionNamingConvention(std::string_view name, bool isStatic,
                                                       std::string_view context, const errors::SourceLocation& location)
    {
        NameValidator::validateFunctionNamingConvention(name, isStatic, context, location);
    }

    std::vector<std::pair<std::string, value::ValueType>> ParserUtils::parseParameterList(
        TokenStream& stream, bool expectParentheses)
    {
        return ParameterParser::parseParameterList(stream, expectParentheses);
    }

    std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>> ParserUtils::parseGenericParameterList(
        TokenStream& stream, bool expectParentheses)
    {
        return ParameterParser::parseGenericParameterList(stream, expectParentheses);
    }

    std::vector<std::pair<std::string, value::ParameterType>> ParserUtils::parseParameterListWithTypes(
        TokenStream& stream, bool expectParentheses)
    {
        return ParameterParser::parseParameterListWithTypes(stream, expectParentheses);
    }

    std::vector<std::string> ParserUtils::parseQualifiedIdentifierChain(
        TokenStream& stream, std::string_view initialName)
    {
        return QualifiedNameParser::parseQualifiedIdentifierChain(stream, initialName);
    }

    void ParserUtils::validateCapitalizedName(std::string_view name,
                                              std::string_view context,
                                              const errors::SourceLocation& location)
    {
        NameValidator::validateCapitalizedName(name, context, location);
    }

    void ParserUtils::validateIdentifierName(std::string_view name,
                                             std::string_view context,
                                             const errors::SourceLocation& location)
    {
        NameValidator::validateIdentifierName(name, context, location);
    }

    // ============================================================================
    // CORE PARSER UTILITIES - Active implementations
    // ============================================================================

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
            errors::SourceLocation opLocation = stream.current().location; // Capture location before advancing
            stream.advance();
            auto right = parseNext();
            left = std::make_unique<ast::nodes::expressions::BinaryExpNode>(
                std::move(left), op, std::move(right), opLocation);
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

    std::vector<std::string> ParserUtils::parseInterfaceList(TokenStream& stream, std::string_view keywordName)
    {
        using namespace token;
        using namespace errors;

        std::vector<std::string> interfaces;

        // Parse first interface
        do
        {
            if (stream.current().type != TokenType::IDENTIFIER)
            {
                std::string message = "Expected interface name after '" + std::string(keywordName) + "'";
                throw ParseException(message, stream.location());
            }

            std::string interfaceName = stream.current().stringValue.getString();
            stream.advance();

            // Handle generic parameters for the interface
            if (stream.current().type == TokenType::LESS)
            {
                interfaceName += parseNestedGenericExpression(stream);
            }

            interfaces.push_back(interfaceName);

            // Check for comma (multiple interfaces)
            if (stream.current().type == TokenType::COMMA)
            {
                stream.advance();
            }
            else
            {
                break;
            }
        }
        while (true);

        return interfaces;
    }

    std::string ParserUtils::parseNestedGenericExpression(TokenStream& stream)
    {
        using namespace token;

        std::string result = "<";
        int depth = 1;
        stream.advance(); // consume '<'

        while (depth > 0 && !stream.isAtEnd())
        {
            if (stream.current().type == TokenType::LESS)
            {
                depth++;
                result += "<";
                stream.advance();
            }
            else if (stream.current().type == TokenType::GREATER)
            {
                depth--;
                result += ">";
                stream.advance();
            }
            // Handle >> as two > tokens for nested generics like Comparable<T>>
            else if (stream.current().type == TokenType::RIGHT_SHIFT)
            {
                // When at depth 1 with >>, we only consume one > (close our level)
                // and leave the second > for the outer context using expectGreaterForGeneric
                if (depth == 1)
                {
                    depth--;
                    result += ">";
                    // Use expectGreaterForGeneric which will set pending flag for the second >
                    stream.expectGreaterForGeneric();
                }
                else
                {
                    // At deeper levels, >> closes two levels
                    depth -= 2;
                    result += ">>";
                    stream.advance();
                }
            }
            else if (stream.current().type == TokenType::IDENTIFIER)
            {
                result += stream.current().stringValue.getString();
                stream.advance();
            }
            else if (stream.current().type == TokenType::COMMA)
            {
                result += ",";
                stream.advance();
            }
            else if (stream.current().type == TokenType::SCOPE)
            {
                result += "::";
                stream.advance();
            }
            // Handle primitive type keywords (needed for validation to work)
            else if (stream.current().type == TokenType::VOID)
            {
                result += "void";
                stream.advance();
            }
            else if (stream.current().type == TokenType::INT)
            {
                result += "int";
                stream.advance();
            }
            else if (stream.current().type == TokenType::FLOAT)
            {
                result += "float";
                stream.advance();
            }
            else if (stream.current().type == TokenType::BOOL)
            {
                result += "bool";
                stream.advance();
            }
            else if (stream.current().type == TokenType::STRING_TYPE)
            {
                result += "string";
                stream.advance();
            }
            else
            {
                // Unknown token, just advance
                stream.advance();
            }
        }

        return result;
    }
}
