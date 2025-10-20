#pragma once
#include "IParser.hpp"
#include "../TokenStream.hpp"
#include "../ParseContext.hpp"
#include "../../token/TokenType.hpp"
#include "../../errors/SourceLocation.hpp"

namespace parser::core
{
    using namespace token;
    using namespace errors;

    class BaseParser : public IParser
    {
    protected:
        TokenStream& tokenStream;
        ParseContext& context;

    public:
        explicit BaseParser(TokenStream& stream, ParseContext& ctx);

        virtual ~BaseParser() = default;

    protected:
        void expectToken(TokenType type);
        bool tryConsumeToken(TokenType type);

        void recoverToToken(TokenType type);
        void recoverToStatement();
        void recoverToBlock();

        bool isAtEnd() const;
        const Token& currentToken() const;
    };
}
