#pragma once
#include "../core/BaseParser.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../ast/nodes/statements/TryNode.hpp"
#include "../../ast/nodes/statements/CatchNode.hpp"
#include "../../ast/nodes/statements/ThrowNode.hpp"
#include <memory>

namespace parser::statement
{
    using namespace ast;
    using namespace parser::core;

    /**
     * @brief Parser for exception handling statements
     *
     * Handles parsing of:
     * - try { ... } catch (Type var) { ... } finally { ... }
     * - throw expression;
     */
    class ExceptionParser : public BaseParser
    {
    public:
        explicit ExceptionParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        std::unique_ptr<TryNode> parseTryStatement();
        std::unique_ptr<ThrowNode> parseThrowStatement();

    private:
        std::unique_ptr<CatchNode> parseCatchClause();
        std::unique_ptr<ASTNode> parseFinallyClause();

        bool isExceptionToken(token::TokenType type) const noexcept;
    };
}
