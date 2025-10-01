#include "ArgumentParser.hpp"
#include "../../errors/ParseException.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::expression
{
    using namespace token;
    using namespace errors;

    std::unique_ptr<ASTNode> ArgumentParser::parse()
    {
        // This should not be called directly - use parseArguments or parseGenericTypeArguments
        reportError("ArgumentParser::parse() called directly", getParserName());
        throw errors::ParseException("Invalid use of ArgumentParser");
    }

    bool ArgumentParser::canParse(const TokenStream& stream) const
    {
        // Arguments can start with any expression token
        return true; // This will be determined by the calling context
    }

    std::vector<std::unique_ptr<ASTNode>> ArgumentParser::parseArguments()
    {
        std::vector<std::unique_ptr<ASTNode>> arguments;

        if (!tokenStream.check(TokenType::RPAREN))
        {
            arguments.push_back(context.parseExpression());

            while (tryConsumeToken(TokenType::COMMA))
            {
                arguments.push_back(context.parseExpression());
            }
        }

        return arguments;
    }

    std::vector<std::string> ArgumentParser::parseGenericTypeArguments()
    {
        std::vector<std::string> typeArgs;

        // Parse first type argument
        typeArgs.push_back(parseGenericTypeArgument());

        // Parse additional arguments separated by commas
        while (tryConsumeToken(TokenType::COMMA))
        {
            typeArgs.push_back(parseGenericTypeArgument());
        }

        return typeArgs;
    }

    std::string ArgumentParser::parseGenericTypeArgument()
    {
        std::string typeArg;

        if (tokenStream.check(TokenType::IDENTIFIER))
        {
            typeArg = tokenStream.current().stringValue.getString();
            tokenStream.advance();

            // Handle nested generics like Array<String>
            if (tokenStream.check(TokenType::LESS))
            {
                typeArg += "<";
                tokenStream.advance(); // consume '<'

                // Recursively parse nested type arguments
                auto nestedArgs = parseGenericTypeArguments();
                for (size_t i = 0; i < nestedArgs.size(); ++i)
                {
                    if (i > 0) typeArg += ",";
                    typeArg += nestedArgs[i];
                }

                expectToken(TokenType::GREATER, getParserName()); // consume '>'
                typeArg += ">";
            }

            return typeArg;
        }
        else
        {
            reportError("Expected type argument", getParserName());
            throw errors::ParseException("Expected type argument");
        }
    }

    std::string ArgumentParser::parseNestedGenericType()
    {
        // This method can be expanded for more complex nested generic type parsing
        return parseGenericTypeArgument();
    }
}