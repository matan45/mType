#pragma once
#include "IParser.hpp"
#include "../TokenStream.hpp"
#include "../ParseContext.hpp"
#include "../error/ErrorHandler.hpp"
#include "../../token/TokenType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <memory>

namespace parser::core
{
    using namespace token;
    using namespace errors;
    using namespace parser::error;

    class BaseParser : public IParser
    {
    protected:
        TokenStream& tokenStream;
        ParseContext& context;
        std::shared_ptr<ErrorHandler> errorHandler;

    public:
        BaseParser(TokenStream& stream, ParseContext& ctx, std::shared_ptr<ErrorHandler> handler)
            : tokenStream(stream), context(ctx), errorHandler(handler) {}

        virtual ~BaseParser() = default;

    protected:
        void expectToken(TokenType type, const std::string& parserContext = "");
        bool tryConsumeToken(TokenType type);
        SourceLocation getCurrentLocation() const noexcept;

        void reportError(const std::string& message, const std::string& parserContext = "") const;
        void reportWarning(const std::string& message, const std::string& parserContext = "") const;

        void recoverToToken(TokenType type);
        void recoverToStatement();
        void recoverToBlock();

        bool isAtEnd() const noexcept { return tokenStream.isAtEnd(); }
        const Token& currentToken() const noexcept { return tokenStream.current(); }
    };
}