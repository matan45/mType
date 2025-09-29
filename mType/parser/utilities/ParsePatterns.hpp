#pragma once
#include "../TokenStream.hpp"
#include "../../token/TokenType.hpp"
#include <vector>
#include <functional>

namespace parser::utilities
{
    using namespace token;

    class ParsePatterns
    {
    public:
        template<typename T>
        static std::vector<T> parseList(
            TokenStream& stream,
            std::function<T()> parseElement,
            TokenType separator,
            TokenType terminator,
            bool allowEmpty = true)
        {
            std::vector<T> elements;

            if (stream.check(terminator))
            {
                return elements;
            }

            elements.push_back(parseElement());

            while (stream.match(separator))
            {
                if (stream.check(terminator) && allowEmpty)
                {
                    break;
                }
                elements.push_back(parseElement());
            }

            return elements;
        }

        static bool isStatementKeyword(TokenType type) noexcept;
        static bool isExpressionStart(TokenType type) noexcept;
        static bool isTypeKeyword(TokenType type) noexcept;

        static void skipToMatchingBrace(TokenStream& stream);
        static void skipToSemicolon(TokenStream& stream);
    };
}