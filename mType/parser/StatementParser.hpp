#pragma once
#include <memory>
#include <vector>
#include "../ast/ASTNode.hpp"
#include "../token/Token.hpp"

namespace parser
{
    class Parser;
    using namespace ast;
    
    class StatementParser
    {
    private:
        Parser& parser;
        
    public:
        explicit StatementParser(Parser& p) : parser(p) {}
        
        // Statement parsing methods
        std::unique_ptr<ASTNode> parseStatement();
        std::unique_ptr<ASTNode> parseBlock();
        std::unique_ptr<ASTNode> parseDeclaration();
        std::unique_ptr<ASTNode> parseAssignment();
        std::unique_ptr<ASTNode> parseIfStatement();
        std::unique_ptr<ASTNode> parseWhileStatement();
        std::unique_ptr<ASTNode> parseDoWhileStatement();
        std::unique_ptr<ASTNode> parseForStatement();
        std::unique_ptr<ASTNode> parseSwitchStatement();
        std::unique_ptr<ASTNode> parseBreakStatement();
        std::unique_ptr<ASTNode> parseContinueStatement();
        std::unique_ptr<ASTNode> parseReturnStatement();
        std::unique_ptr<ASTNode> parseExpressionStatement();
        std::unique_ptr<ASTNode> parseFunction();
        std::unique_ptr<ASTNode> parseImport();
        std::unique_ptr<ASTNode> parseNativeFunction();
        
    private:
        //TODO move to utils
        // Helper methods
        ValueType parseType();
        std::vector<std::pair<std::string, ValueType>> parseParameterList();
    };
}

