#pragma once
#include "../core/BaseParser.hpp"
#include "../../ast/ASTNode.hpp"
#include <memory>
#include <vector>
#include <functional>

namespace parser
{
    class ExpressionParser; // Forward declaration
}

namespace parser::expression
{
    using namespace ast;
    using namespace parser::core;

    class BinaryOperatorParser : public BaseParser
    {
    private:
        parser::ExpressionParser* expressionParser; // Reference to ExpressionParser to break circular dependency

    public:
        BinaryOperatorParser(TokenStream& stream, ParseContext& ctx, std::shared_ptr<error::ErrorHandler> handler)
            : BaseParser(stream, ctx, handler), expressionParser(nullptr) {}

        // Method to set ExpressionParser reference after construction
        void setExpressionParser(parser::ExpressionParser& exprParser)
        {
            expressionParser = &exprParser;
        }

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;
        std::string getParserName() const override { return "BinaryOperatorParser"; }

        // Precedence climbing methods
        std::unique_ptr<ASTNode> parseTernary();
        std::unique_ptr<ASTNode> parseLogicalOr();
        std::unique_ptr<ASTNode> parseLogicalAnd();
        std::unique_ptr<ASTNode> parseEquality();
        std::unique_ptr<ASTNode> parseComparison();
        std::unique_ptr<ASTNode> parseAdditive();
        std::unique_ptr<ASTNode> parseMultiplicative();

    private:
        std::unique_ptr<ASTNode> parseBinaryLevel(
            std::function<std::unique_ptr<ASTNode>()> parseNext,
            const std::vector<token::TokenType>& operators);

        bool isBinaryOperator(token::TokenType type) const noexcept;
        int getOperatorPrecedence(token::TokenType type) const noexcept;
    };
}