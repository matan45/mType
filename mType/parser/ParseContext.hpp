#pragma once
#include "../ast/ASTNode.hpp"
#include <memory>

namespace parser
{
    class StatementParser;
    class ExpressionParser;
    class ClassParser;
    class TokenStream;
    
    using namespace ast;

    /// @brief Enables inter-parser communication without circular dependencies
    /// Provides clean interface for parser delegation
    class ParseContext
    {
    private:
        StatementParser* statementParser;
        ExpressionParser* expressionParser;
        ClassParser* classParser;
        TokenStream* tokenStream;

    public:
        ParseContext(StatementParser* stmt, ExpressionParser* expr, ClassParser* cls, TokenStream* stream);
        ~ParseContext() = default;

        /// @brief Parse a statement using StatementParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseStatement();
        
        /// @brief Parse an expression using ExpressionParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseExpression();
        
        /// @brief Parse a class using ClassParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseClass();
        
        /// @brief Parse a new expression using ClassParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseNewExpression();
        
        // Setters for delayed initialization
        void setStatementParser(StatementParser* parser) { statementParser = parser; }
        void setExpressionParser(ExpressionParser* parser) { expressionParser = parser; }
        void setClassParser(ClassParser* parser) { classParser = parser; }
        void setTokenStream(TokenStream* stream) { tokenStream = stream; }
    };
}