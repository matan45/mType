#pragma once
#include "../core/BaseParser.hpp"
#include "../../ast/ASTNode.hpp"
#include <memory>

namespace parser::expression
{
    using namespace ast;
    using namespace parser::core;

    class UnaryOperatorParser : public BaseParser
    {
    public:
        UnaryOperatorParser(TokenStream& stream, ParseContext& ctx, std::shared_ptr<error::ErrorHandler> handler)
            : BaseParser(stream, ctx, handler) {}

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;
        std::string getParserName() const override { return "UnaryOperatorParser"; }

        std::unique_ptr<ASTNode> parseUnary();

    private:
        bool isPrefixUnaryOperator(token::TokenType type) const noexcept;
    };
}