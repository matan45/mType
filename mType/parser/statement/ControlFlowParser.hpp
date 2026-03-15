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
        explicit ControlFlowParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        std::unique_ptr<ASTNode> parseIfStatement();
        std::unique_ptr<ASTNode> parseSwitchStatement();
        std::unique_ptr<ASTNode> parseMatchStatement();
        std::unique_ptr<ASTNode> parseBreakStatement();
        std::unique_ptr<ASTNode> parseContinueStatement();
        std::unique_ptr<ASTNode> parseReturnStatement();

    private:
        bool isControlFlowToken(TokenType type) const noexcept;
        bool isTypePatternStart() const;
    };
}
