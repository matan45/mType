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
        ExpressionParser* expressionParser; // Reference to ExpressionParser to break circular dependency
    public:
        explicit BinaryOperatorParser(TokenStream& stream, ParseContext& ctx);
        
        // Method to set ExpressionParser reference after construction
        void setExpressionParser(ExpressionParser& exprParser);
        
        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        // Precedence climbing methods
        std::unique_ptr<ASTNode> parseTernary();
        std::unique_ptr<ASTNode> parseLogicalOr();
        std::unique_ptr<ASTNode> parseLogicalAnd();
        std::unique_ptr<ASTNode> parseEquality();
        std::unique_ptr<ASTNode> parseComparison();
        std::unique_ptr<ASTNode> parseIsClassOf();
        std::unique_ptr<ASTNode> parseAdditive();
        std::unique_ptr<ASTNode> parseMultiplicative();

    private:
        std::unique_ptr<ASTNode> parseBinaryLevel(
            std::function<std::unique_ptr<ASTNode>()> parseNext,
            const std::vector<TokenType>& operators);

        bool isBinaryOperator(TokenType type) const noexcept;
        int getOperatorPrecedence(TokenType type) const noexcept;
    };
}