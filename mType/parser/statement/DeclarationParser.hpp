#pragma once
#include "../core/BaseParser.hpp"
#include "../../ast/ASTNode.hpp"
#include <memory>

namespace parser::statement
{
    using namespace ast;
    using namespace parser::core;

    class DeclarationParser : public BaseParser
    {
    public:
        DeclarationParser(TokenStream& stream, ParseContext& ctx, std::shared_ptr<error::ErrorHandler> handler)
            : BaseParser(stream, ctx, handler) {}

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;
        std::string getParserName() const override { return "DeclarationParser"; }

        std::unique_ptr<ASTNode> parseDeclaration();

    private:
        bool isDeclarationStart(token::TokenType type) const noexcept;
        bool isModifierToken(token::TokenType type) const noexcept;
        bool isTypeToken(token::TokenType type) const noexcept;

        struct ModifierInfo
        {
            bool isFinal = false;
            bool isStatic = false;
        };

        ModifierInfo parseModifiers();
    };
}