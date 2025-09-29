#pragma once
#include "../core/BaseParser.hpp"
#include "../../ast/ASTNode.hpp"
#include <memory>

namespace parser::statement
{
    using namespace ast;
    using namespace parser::core;

    class ImportParser : public BaseParser
    {
    public:
        ImportParser(TokenStream& stream, ParseContext& ctx, std::shared_ptr<error::ErrorHandler> handler)
            : BaseParser(stream, ctx, handler) {}

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;
        std::string getParserName() const override { return "ImportParser"; }

        std::unique_ptr<ASTNode> parseImport();

    private:
        bool isImportToken(token::TokenType type) const noexcept;
        void validateImportPath(const std::string& path);
    };
}