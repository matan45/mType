#pragma once
#include "../core/BaseParser.hpp"
#include "../../ast/ASTNode.hpp"
#include <memory>
#include <vector>
#include <string>

namespace parser
{
    class ExpressionParser; // Forward declaration
}

namespace parser::expression
{
    using namespace ast;
    using namespace parser::core;

    class PostfixOperatorParser : public BaseParser
    {
    private:
        parser::ExpressionParser* expressionParser; // Reference to ExpressionParser to break circular dependency

    public:
        PostfixOperatorParser(TokenStream& stream, ParseContext& ctx, std::shared_ptr<error::ErrorHandler> handler)
            : BaseParser(stream, ctx, handler), expressionParser(nullptr) {}

        // Method to set ExpressionParser reference after construction
        void setExpressionParser(parser::ExpressionParser& exprParser)
        {
            expressionParser = &exprParser;
        }

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;
        std::string getParserName() const override { return "PostfixOperatorParser"; }

        std::unique_ptr<ASTNode> parsePostfix();
        std::unique_ptr<ASTNode> parsePostfixOperations(std::unique_ptr<ASTNode> expr);
        std::unique_ptr<ASTNode> parseMemberAccess(std::unique_ptr<ASTNode> object);
        std::unique_ptr<ASTNode> parseIndexAccess(std::unique_ptr<ASTNode> collection);

    private:
        std::unique_ptr<ASTNode> parseFunctionCall(std::unique_ptr<ASTNode> expr);
        std::unique_ptr<ASTNode> parseScopeResolution(std::unique_ptr<ASTNode> expr);
        std::unique_ptr<ASTNode> parseGenericMethodCall(const std::string& className,
                                                       const std::string& methodName,
                                                       const std::vector<std::string>& typeArgs);

        bool isPostfixOperator(token::TokenType type) const noexcept;
    };
}