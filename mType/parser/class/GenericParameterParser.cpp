#include "GenericParameterParser.hpp"
#include "../../errors/ParseException.hpp"

namespace parser
{
    using namespace token;
    using namespace errors;

    GenericParameterParser::GenericParameterParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }

    std::unique_ptr<ASTNode> GenericParameterParser::parse()
    {
        // This parser is primarily a utility parser and doesn't create standalone AST nodes
        throw ParseException("GenericParameterParser is a utility parser", tokenStream.current().location);
    }

    bool GenericParameterParser::canParse(const TokenStream& stream) const
    {
        return stream.check(TokenType::LESS);
    }

    std::string GenericParameterParser::parseGenericParameters()
    {
        std::string result;

        // Parse first parameter
        result += parseGenericParameter();

        // Parse additional parameters separated by commas
        while (tokenStream.check(TokenType::COMMA))
        {
            result += ", ";
            tokenStream.advance(); // consume ','
            result += parseGenericParameter();
        }

        return result;
    }

    std::string GenericParameterParser::parseGenericParameter()
    {
        // Handle type name (could be identifier or built-in type)
        if (tokenStream.current().type == TokenType::IDENTIFIER ||
            tokenStream.current().type == TokenType::INT ||
            tokenStream.current().type == TokenType::FLOAT ||
            tokenStream.current().type == TokenType::BOOL ||
            tokenStream.current().type == TokenType::STRING_TYPE)
        {
            std::string paramType = tokenStream.current().stringValue.getString();
            tokenStream.advance();

            // Check for nested generics
            if (tokenStream.check(TokenType::LESS))
            {
                paramType += "<";
                tokenStream.advance(); // consume '<'
                paramType += parseGenericParameters(); // Recursive call
                tokenStream.expect(TokenType::GREATER); // consume '>'
                paramType += ">";
            }

            return paramType;
        }

        throw ParseException("Expected type parameter", tokenStream.current().location);
    }

    std::vector<GenericTypeParameter> GenericParameterParser::parseGenericTypeParameters()
    {
        std::vector<GenericTypeParameter> parameters;

        // Parse first parameter
        parameters.push_back(parseGenericTypeParameter());

        // Parse additional parameters separated by commas
        while (tokenStream.check(TokenType::COMMA))
        {
            tokenStream.advance(); // consume ','
            parameters.push_back(parseGenericTypeParameter());
        }

        return parameters;
    }

    GenericTypeParameter GenericParameterParser::parseGenericTypeParameter()
    {
        // Expect an identifier for the type parameter name
        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected generic type parameter name", tokenStream.current().location);
        }

        std::string paramName = tokenStream.current().stringValue.getString();
        auto location = tokenStream.current().location;
        validateGenericParameterName(paramName);
        tokenStream.advance();

        // Parse optional constraints (extends/implements SomeInterface)
        std::vector<std::string> constraints;
        if (tokenStream.current().type == TokenType::EXTENDS ||
            tokenStream.current().type == TokenType::IMPLEMENTS)
        {
            tokenStream.advance(); // consume constraint keyword

            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected interface name after constraint keyword",
                                     tokenStream.current().location);
            }

            std::string constraintName = tokenStream.current().stringValue.getString();
            tokenStream.advance();

            // Handle generic parameters in constraints (e.g., Comparable<T>)
            if (tokenStream.current().type == TokenType::LESS)
            {
                constraintName += parseNestedGenericConstraint();
            }

            constraints.push_back(constraintName);

            // VALIDATION: Each generic parameter can only extend ONE interface
            // Check for additional extends/implements keywords
            if (tokenStream.current().type == TokenType::EXTENDS ||
                tokenStream.current().type == TokenType::IMPLEMENTS)
            {
                throw ParseException(
                    "Generic type parameter '" + paramName + "' can only extend one interface. "
                    "Multiple interface constraints are not supported.",
                    tokenStream.current().location);
            }
        }

        return GenericTypeParameter(paramName, constraints, location);
    }

    std::string GenericParameterParser::parseNestedGenericType()
    {
        std::string result = parseGenericParameter();

        while (tokenStream.check(TokenType::COMMA))
        {
            result += ", ";
            tokenStream.advance();
            result += parseGenericParameter();
        }

        return result;
    }

    void GenericParameterParser::validateGenericParameterName(const std::string& paramName)
    {
        // Generic type parameters should typically be single uppercase letters (T, U, K, etc.)
        // But we'll be flexible and just ensure it's a valid identifier
        if (paramName.empty())
        {
            throw ParseException("Generic type parameter name cannot be empty", tokenStream.current().location);
        }
    }

    std::string GenericParameterParser::parseNestedGenericConstraint()
    {
        std::string result = "<";
        int depth = 1;
        tokenStream.advance(); // consume '<'

        while (depth > 0 && !tokenStream.isAtEnd())
        {
            if (tokenStream.current().type == TokenType::LESS)
            {
                depth++;
                result += "<";
            }
            else if (tokenStream.current().type == TokenType::GREATER)
            {
                depth--;
                result += ">";
            }
            else if (tokenStream.current().type == TokenType::IDENTIFIER)
            {
                result += tokenStream.current().stringValue.getString();
            }
            else if (tokenStream.current().type == TokenType::COMMA)
            {
                result += ",";
            }
            tokenStream.advance();
        }

        return result;
    }
}
