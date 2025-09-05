#pragma once
#include "../ast/ASTNode.hpp"
#include <memory>
#include <optional>
#include <functional>

namespace parser
{
    class StatementParser;
    class ExpressionParser;
    class ClassParser;
    class TokenStream;
    
    using namespace ast;

    /// @brief Enables inter-parser communication without circular dependencies
    /// Provides clean interface for parser delegation with memory-safe references
    class ParseContext
    {
    private:
        std::optional<std::reference_wrapper<StatementParser>> statementParser;
        std::optional<std::reference_wrapper<ExpressionParser>> expressionParser;
        std::optional<std::reference_wrapper<ClassParser>> classParser;
        std::optional<std::reference_wrapper<TokenStream>> tokenStream;

    public:
        /// @brief Default constructor for delayed initialization
        ParseContext() = default;
        
        /// @brief Constructor with immediate initialization
        ParseContext(StatementParser& stmt, ExpressionParser& expr, ClassParser& cls, TokenStream& stream);
        
        ~ParseContext() = default;

        /// @brief Parse a statement using StatementParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseStatement();
        
        /// @brief Parse an expression using ExpressionParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseExpression();
        
        /// @brief Parse a class using ClassParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseClass();
        
        /// @brief Parse a new expression using ClassParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseNewExpression();
        
        // Setters for delayed initialization with memory-safe references
        void setStatementParser(StatementParser& parser) { statementParser = std::ref(parser); }
        void setExpressionParser(ExpressionParser& parser) { expressionParser = std::ref(parser); }
        void setClassParser(ClassParser& parser) { classParser = std::ref(parser); }
        void setTokenStream(TokenStream& stream) { tokenStream = std::ref(stream); }
    };
}