#pragma once
#include "../core/BaseParser.hpp"
#include "../../ast/ASTNode.hpp"
#include <memory>

namespace parser
{
    class ExpressionParser; // Forward declaration
}

namespace parser::expression
{
    using namespace ast;
    using namespace parser::core;

    /**
     * @brief Parses cast expressions and isClassOf operations
     *
     * Responsibilities:
     * - Parse (Type)expression cast syntax
     * - Parse expression isClassOf Type syntax
     *
     * Design Principles:
     * - Single Responsibility: Only type casting/checking
     * - Integrates with TypeParser for type parsing
     * - Delegates to ExpressionParser for operand parsing
     */
    class CastParser : public BaseParser
    {
    private:
        ExpressionParser* expressionParser;

    public:
        explicit CastParser(TokenStream& stream, ParseContext& ctx);

        void setExpressionParser(ExpressionParser& exprParser);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        // Parse cast: (Type)expression
        std::unique_ptr<ASTNode> parseCastExpression();

        // Parse isClassOf: expression isClassOf Type
        std::unique_ptr<ASTNode> parseInstanceOfExpression(std::unique_ptr<ASTNode> left);

        // Check if current position could be a cast (not a parenthesized expression)
        bool isCastExpression() const;

    private:
        // Helper to distinguish cast from parenthesized expression
    };
}
