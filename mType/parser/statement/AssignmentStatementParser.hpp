#pragma once
#include "../core/BaseParser.hpp"
#include "../../ast/ASTNode.hpp"
#include <memory>

namespace parser
{
    class ExpressionParser; // Forward declaration
}

namespace parser::statement
{
    using namespace ast;
    using namespace parser::core;

    class AssignmentStatementParser : public BaseParser
    {
    private:
        parser::ExpressionParser* expressionParser; // Reference to ExpressionParser to break circular dependency

    public:
        AssignmentStatementParser(TokenStream& stream, ParseContext& ctx, std::shared_ptr<error::ErrorHandler> handler)
            : BaseParser(stream, ctx, handler), expressionParser(nullptr) {}

        // Method to set ExpressionParser reference after construction
        void setExpressionParser(parser::ExpressionParser& exprParser)
        {
            expressionParser = &exprParser;
        }

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;
        std::string getParserName() const override { return "AssignmentStatementParser"; }

        std::unique_ptr<ASTNode> parseAssignment();
        std::unique_ptr<ASTNode> parseExpressionStatement();

    private:
        bool isAssignmentOperator(token::TokenType type) const noexcept;
        token::TokenType getCorrespondingBinaryOperator(token::TokenType assignmentOp) const;
        std::unique_ptr<ASTNode> createCompoundAssignment(const std::string& varName,
                                                          const errors::SourceLocation& location,
                                                          token::TokenType opType,
                                                          std::unique_ptr<ASTNode> value);
    };
}