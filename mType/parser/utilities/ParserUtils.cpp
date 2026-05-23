#include "ParserUtils.hpp"
#include <algorithm>
#include "../TokenStream.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../errors/ParseException.hpp"
#include "../../errors/SourceLocation.hpp"

namespace parser
{
    void ParserUtils::throwPrimitiveInGeneric(
        TokenStream& stream,
        std::string_view primitiveName,
        std::string_view boxedName)
    {
        using namespace errors;
        throw ParseException(
            "Primitive type '" + std::string(primitiveName) +
            "' cannot be used as generic type argument. Use boxed type '" +
            std::string(boxedName) + "' instead",
            stream.location());
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
            errors::SourceLocation opLocation = stream.current().location;
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

        do
        {
            if (stream.current().type != TokenType::IDENTIFIER)
            {
                std::string message = "Expected interface name after '" + std::string(keywordName) + "'";
                throw ParseException(message, stream.location());
            }

            std::string interfaceName = std::string(stream.current().stringValue);
            stream.advance();

            if (stream.current().type == TokenType::LESS)
            {
                interfaceName += parseNestedGenericExpression(stream);
            }

            interfaces.push_back(interfaceName);

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
        stream.advance();

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
            else if (stream.current().type == TokenType::RIGHT_SHIFT)
            {
                if (depth == 1)
                {
                    depth--;
                    result += ">";
                    stream.expectGreaterForGeneric();
                }
                else
                {
                    depth -= 2;
                    result += ">>";
                    stream.advance();
                }
            }
            else if (stream.current().type == TokenType::IDENTIFIER)
            {
                result += std::string(stream.current().stringValue);
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
            else if (stream.current().type == TokenType::VOID)
            {
                result += "void";
                stream.advance();
            }
            else if (stream.current().type == TokenType::INT)
            {
                throwPrimitiveInGeneric(stream, "int", "Int");
            }
            else if (stream.current().type == TokenType::FLOAT)
            {
                throwPrimitiveInGeneric(stream, "float", "Float");
            }
            else if (stream.current().type == TokenType::BOOL)
            {
                throwPrimitiveInGeneric(stream, "bool", "Bool");
            }
            else if (stream.current().type == TokenType::STRING_TYPE)
            {
                throwPrimitiveInGeneric(stream, "string", "String");
            }
            else
            {
                stream.advance();
            }
        }

        return result;
    }
}
