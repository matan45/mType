#pragma once
#include "../core/BaseParser.hpp"
#include "../../ast/ASTNode.hpp"
#include <memory>

namespace parser::statement
{
    using namespace ast;
    using namespace parser::core;

    class LoopParser : public BaseParser
    {
    public:
        explicit LoopParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        std::unique_ptr<ASTNode> parseWhileStatement();
        std::unique_ptr<ASTNode> parseDoWhileStatement();
        std::unique_ptr<ASTNode> parseForStatement();
        std::unique_ptr<ASTNode> parseForEachStatement();

    private:
        bool isLoopToken(token::TokenType type) const noexcept;
        std::unique_ptr<ASTNode> tryParseForEach();
    };
}