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
        explicit ImportParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        std::unique_ptr<ASTNode> parseImport();

    private:
        bool isImportToken(TokenType type) const noexcept;
        void validateImportPath(const std::string& path);
    };
}