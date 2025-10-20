#pragma once
#include "../core/BaseParser.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../errors/SourceLocation.hpp"
#include <memory>

namespace parser::statement
{
    using namespace ast;
    using namespace parser::core;
    using namespace errors;

    class ImportParser : public BaseParser
    {
    public:
        explicit ImportParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        std::unique_ptr<ASTNode> parseImport();

    private:
        // Parse selective import: import {A, B, C} from "file.mt"
        std::unique_ptr<ASTNode> parseSelectiveImport(const SourceLocation& loc);

        // Parse wildcard import: import * from "file.mt"
        std::unique_ptr<ASTNode> parseWildcardImport(const SourceLocation& loc);

        bool isImportToken(TokenType type) const noexcept;
        void validateImportPath(const std::string& path);
    };
}
