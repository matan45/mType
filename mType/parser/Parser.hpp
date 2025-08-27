#pragma once
#include "../lexer/Lexer.hpp"
#include "../ast/ASTNode.hpp"
#include "StatementParser.hpp"
#include "ExpressionParser.hpp"
#include "NamespaceParser.hpp"
#include "ClassParser.hpp"

namespace parser
{
    using namespace lexer;
    using namespace ast;
    class Parser
    {
    private:
        Lexer& lexer;
        Token currentToken;
        
        // Subparsers
        std::unique_ptr<StatementParser> statementParser;
        std::unique_ptr<ExpressionParser> expressionParser;
        std::unique_ptr<NamespaceParser> namespaceParser;
        std::unique_ptr<ClassParser> classParser;
    public:
        explicit Parser(Lexer& lex);
        ~Parser() = default;
        std::unique_ptr<ASTNode> parseProgram();

        // Getters for subparsers (for inter-parser communication)
        StatementParser* getStatementParser() { return statementParser.get(); }
        ExpressionParser* getExpressionParser() { return expressionParser.get(); }
        NamespaceParser* getNamespaceParser() { return namespaceParser.get(); }
        ClassParser* getClassParser() { return classParser.get(); }
    };
}

