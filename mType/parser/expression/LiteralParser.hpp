#pragma once
#include "../core/BaseParser.hpp"
#include "../../ast/ASTNode.hpp"
#include <memory>

namespace parser::expression
{
    using namespace ast;
    using namespace parser::core;

    class LiteralParser : public BaseParser
    {
    public:
        explicit LiteralParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        std::unique_ptr<ASTNode> parsePrimary();
        std::unique_ptr<ASTNode> parseArrayLiteral();

    private:
        std::unique_ptr<ASTNode> parseIntegerLiteral();
        std::unique_ptr<ASTNode> parseFloatLiteral();
        std::unique_ptr<ASTNode> parseStringLiteral();
        std::unique_ptr<ASTNode> parseBooleanLiteral();
        std::unique_ptr<ASTNode> parseNullLiteral();
        std::unique_ptr<ASTNode> parseIdentifier();
        std::unique_ptr<ASTNode> parseParenthesizedExpression();

        bool isLiteralToken(TokenType type) const noexcept;
    };
}