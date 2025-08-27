#include "Parser.hpp"

namespace parser
{
    Parser::Parser(Lexer& lex) 
        : lexer(lex), currentToken(lexer.getNextToken())
    {
        // Initialize subparsers with reference to main parser
        statementParser = std::make_unique<StatementParser>(*this);
        expressionParser = std::make_unique<ExpressionParser>(*this);
        namespaceParser = std::make_unique<NamespaceParser>(*this);
        classParser = std::make_unique<ClassParser>(*this);
    }

    std::unique_ptr<ASTNode> Parser::parseProgram()
    {
        std::vector<std::unique_ptr<ASTNode>> statements;
        while (currentToken.type != TokenType::END)
        {
            statements.push_back(statementParser->parseStatement());
        }
        //return new ProgramNode(statements, currentToken.location);
        return nullptr;
    }
}
