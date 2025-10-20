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
        explicit DeclarationParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        std::unique_ptr<ASTNode> parseDeclaration();

    private:
        bool isDeclarationStart(TokenType type) const noexcept;
        bool isModifierToken(TokenType type) const noexcept;
        bool isTypeToken(TokenType type) const noexcept;

        struct ModifierInfo
        {
            bool isFinal = false;
            bool isStatic = false;
        };

        ModifierInfo parseModifiers();
    };
}
