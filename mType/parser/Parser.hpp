#pragma once
#include "../lexer/Lexer.hpp"
#include "../ast/ASTNode.hpp"
#include "../token/TokenType.hpp"
#include "StatementParser.hpp"
#include "ExpressionParser.hpp"
#include "NamespaceParser.hpp"
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
        services::ImportManager* importManager;
        
        // Subparsers
        std::unique_ptr<StatementParser> statementParser;
        std::unique_ptr<ExpressionParser> expressionParser;
        std::unique_ptr<NamespaceParser> namespaceParser;
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
        explicit Parser(Lexer& lex);
        ~Parser() = default;
        
        void setImportManager(services::ImportManager* manager) { importManager = manager; }
        services::ImportManager* getImportManager() { return importManager; }
        
        std::unique_ptr<ASTNode> parseProgram();
        std::unique_ptr<ASTNode> parseStatement();
        std::unique_ptr<ASTNode> parseExpression();

        // Getters for subparsers (for inter-parser communication)
        StatementParser* getStatementParser() { return statementParser.get(); }
        ExpressionParser* getExpressionParser() { return expressionParser.get(); }
        NamespaceParser* getNamespaceParser() { return namespaceParser.get(); }
        ClassParser* getClassParser() { return classParser.get(); }
        
        // Access to current token for subparsers
        const Token& getCurrentToken() const { return currentToken; }
        void advanceToken() { advance(); }
        bool matchToken(TokenType type) { return match(type); }
        void expectToken(TokenType type) { expect(type); }
    };
}

