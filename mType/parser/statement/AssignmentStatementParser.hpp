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
        ExpressionParser* expressionParser; // Reference to ExpressionParser to break circular dependency

    public:
        explicit AssignmentStatementParser(TokenStream& stream, ParseContext& ctx);

        // Method to set ExpressionParser reference after construction
        void setExpressionParser(ExpressionParser& exprParser);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        std::unique_ptr<ASTNode> parseAssignment();
        std::unique_ptr<ASTNode> parseExpressionStatement();

    private:
        bool isAssignmentOperator(TokenType type) const noexcept;
        TokenType getCorrespondingBinaryOperator(TokenType assignmentOp) const;
        std::unique_ptr<ASTNode> createCompoundAssignment(const std::string& varName,
                                                          const SourceLocation& location,
                                                          TokenType opType,
                                                          std::unique_ptr<ASTNode> value);
    };
}