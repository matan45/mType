#include "AccessModifierParser.hpp"
#include "../../errors/ParseException.hpp"
#include <stdexcept>

namespace parser::utilities
{
    AccessModifier AccessModifierParser::parseAccessModifier(
        TokenStream& tokenStream,
        AccessModifier defaultModifier)
    {
        const auto& currentToken = tokenStream.current();

        if (isAccessModifier(currentToken.type))
        {
            AccessModifier modifier = tokenTypeToAccessModifier(currentToken.type);
            tokenStream.advance(); // Consume the modifier token
            return modifier;
        }

        return defaultModifier;
    }

    bool AccessModifierParser::isAccessModifier(TokenType type)
    {
        return type == TokenType::PRIVATE ||
            type == TokenType::PUBLIC ||
            type == TokenType::PROTECTED;
    }

    void AccessModifierParser::validateModifierForContext(
        AccessModifier modifier,
        bool isInterface,
        const SourceLocation& location)
    {
        if (isInterface && modifier != AccessModifier::PUBLIC)
        {
            throw ParseException(
                "Interface methods must be public. Private and protected modifiers are not allowed in interfaces.",
                location);
        }
    }

    AccessModifier AccessModifierParser::tokenTypeToAccessModifier(TokenType type)
    {
        switch (type)
        {
        case TokenType::PRIVATE:
            return AccessModifier::PRIVATE;
        case TokenType::PUBLIC:
            return AccessModifier::PUBLIC;
        case TokenType::PROTECTED:
            return AccessModifier::PROTECTED;
        default:
            throw std::invalid_argument("Token type is not an access modifier");
        }
    }
}
