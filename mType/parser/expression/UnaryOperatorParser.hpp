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

    class UnaryOperatorParser : public BaseParser
    {
    private:
        ExpressionParser* expressionParser; // Reference to ExpressionParser to break circular dependency

    public:
        explicit UnaryOperatorParser(TokenStream& stream, ParseContext& ctx);


        // Method to set ExpressionParser reference after construction
        void setExpressionParser(ExpressionParser& exprParser);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        std::unique_ptr<ASTNode> parseUnary();

    private:
        bool isPrefixUnaryOperator(token::TokenType type) const noexcept;
    };
}
