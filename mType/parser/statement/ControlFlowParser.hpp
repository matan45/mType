#pragma once
#include "../core/BaseParser.hpp"
#include "../../ast/ASTNode.hpp"
#include <memory>

namespace parser::statement
{
    using namespace ast;
    using namespace parser::core;

    class ControlFlowParser : public BaseParser
    {
    public:
        ControlFlowParser(TokenStream& stream, ParseContext& ctx, std::shared_ptr<error::ErrorHandler> handler)
            : BaseParser(stream, ctx, handler) {}

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;
        std::string getParserName() const override { return "ControlFlowParser"; }

        std::unique_ptr<ASTNode> parseIfStatement();
        std::unique_ptr<ASTNode> parseSwitchStatement();
        std::unique_ptr<ASTNode> parseBreakStatement();
        std::unique_ptr<ASTNode> parseContinueStatement();
        std::unique_ptr<ASTNode> parseReturnStatement();

    private:
        bool isControlFlowToken(token::TokenType type) const noexcept;
    };
}