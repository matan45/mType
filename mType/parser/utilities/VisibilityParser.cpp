#include "VisibilityParser.hpp"
#include <stdexcept>

namespace parser::utilities
{
    VisibilityModifier VisibilityParser::parseVisibilityModifier(
        TokenStream& tokenStream,
        VisibilityModifier defaultModifier)
    {
        const auto& currentToken = tokenStream.current();

        if (isVisibilityModifier(currentToken.type))
        {
            VisibilityModifier modifier = tokenTypeToVisibilityModifier(currentToken.type);
            tokenStream.advance(); // Consume the modifier token
            return modifier;
        }

        // No visibility modifier found, return default (PUBLIC)
        return defaultModifier;
    }

    bool VisibilityParser::isVisibilityModifier(TokenType type)
    {
        return type == TokenType::PRIVATE || type == TokenType::PUBLIC;
    }

    VisibilityModifier VisibilityParser::tokenTypeToVisibilityModifier(TokenType type)
    {
        switch (type)
        {
        case TokenType::PRIVATE:
            return VisibilityModifier::PRIVATE;
        case TokenType::PUBLIC:
            return VisibilityModifier::PUBLIC;
        default:
            throw std::invalid_argument("Token type is not a visibility modifier");
        }
    }
}
