#include "ArgumentParser.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::expression
{
    using namespace token;
    using namespace errors;

    ArgumentParser::ArgumentParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }


    std::unique_ptr<ASTNode> ArgumentParser::parse()
    {
        throw ParseException("Invalid use of ArgumentParser", tokenStream.current().location);
    }

    bool ArgumentParser::canParse(const TokenStream& stream) const
    {
        return true;
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

    std::vector<std::unique_ptr<ASTNode>> ArgumentParser::parseArgumentsWithParentheses()
    {
        expectToken(TokenType::LPAREN);
        auto arguments = parseArguments();
        expectToken(TokenType::RPAREN);
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

                tokenStream.expectGreaterForGeneric(); // consume '>' (handles >> for nested generics)
                typeArg += ">";
            }

            return typeArg;
        }

        throw ParseException("Expected type argument", tokenStream.current().location);
    }

    std::string ArgumentParser::parseNestedGenericType()
    {
        // This method can be expanded for more complex nested generic type parsing
        return parseGenericTypeArgument();
    }
}
