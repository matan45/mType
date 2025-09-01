#pragma once
#include "../lexer/Lexer.hpp"
#include "../ast/ASTNode.hpp"
#include "../token/TokenType.hpp"
#include "../value/ValueType.hpp"
#include "StatementParser.hpp"
#include "ExpressionParser.hpp"
#include "ClassParser.hpp"
#include <memory>

namespace services
{
    class ImportManager;
}

namespace parser
{
    using namespace lexer;
    using namespace ast;
    using namespace token;

    class Parser
    {
    private:
        Lexer& lexer;
        Token currentToken;
        std::unique_ptr<services::ImportManager> importManager;

        // Subparsers
        std::unique_ptr<StatementParser> statementParser;
        std::unique_ptr<ExpressionParser> expressionParser;
        std::unique_ptr<ClassParser> classParser;

        // Helper methods for subparsers to use
        void advance();
        bool match(TokenType type);
        void expect(TokenType type);

        // Import handling
        void handleImportStatement(nodes::statements::ImportNode* importNode,
                                   nodes::statements::ProgramNode* program);
        bool isImportableDeclaration(ASTNode* node);

    public:
        explicit Parser(Lexer& lex, std::unique_ptr<services::ImportManager> manager);
        ~Parser() = default;

        std::unique_ptr<services::ImportManager> getImportManager() { return std::move(importManager); }

        std::unique_ptr<ASTNode> parseProgram();
        std::unique_ptr<ASTNode> parseStatement();
        std::unique_ptr<ASTNode> parseExpression();

        // Getters for subparsers (for inter-parser communication)
        StatementParser* getStatementParser() { return statementParser.get(); }
        ExpressionParser* getExpressionParser() { return expressionParser.get(); }
        ClassParser* getClassParser() { return classParser.get(); }

        // Access to current token for subparsers
        const Token& getCurrentToken() const { return currentToken; }
        void advanceToken() { advance(); }
        bool matchToken(TokenType type) { return match(type); }
        void expectToken(TokenType type) { expect(type); }
        Token peekNextToken() { return lexer.peekNextToken(); }

        // Common utility methods
        static bool isAssignmentOperator(TokenType tokenType);
        ValueType parseType();

        // Type parsing with class name for objects
        std::pair<ValueType, std::string> parseTypeWithClassName();
    };
}
