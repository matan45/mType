#pragma once
#include <memory>
#include <vector>
#include "../ast/ASTNode.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"

namespace parser
{
    class ParseContext;
    using namespace ast;
    
    class StatementParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;
        
    public:
        explicit StatementParser(TokenStream& stream, ParseContext& ctx) : tokenStream(stream), context(ctx) {}
        
        // Statement parsing methods
        std::unique_ptr<ASTNode> parseStatement();
        std::unique_ptr<ASTNode> parseBlock();
        std::unique_ptr<ASTNode> parseDeclaration();
        std::unique_ptr<ASTNode> parseAssignment();
        std::unique_ptr<ASTNode> parseIfStatement();
        std::unique_ptr<ASTNode> parseWhileStatement();
        std::unique_ptr<ASTNode> parseDoWhileStatement();
        std::unique_ptr<ASTNode> parseForStatement();
        std::unique_ptr<ASTNode> parseForEachStatement();
        std::unique_ptr<ASTNode> parseSwitchStatement();
        std::unique_ptr<ASTNode> parseBreakStatement();
        std::unique_ptr<ASTNode> parseContinueStatement();
        std::unique_ptr<ASTNode> parseReturnStatement();
        std::unique_ptr<ASTNode> parseExpressionStatement();
        std::unique_ptr<ASTNode> parseFunction();
        std::unique_ptr<ASTNode> parseImport();
        std::unique_ptr<ASTNode> parseNativeFunction();
        
    private:
        // Helper methods
        std::vector<std::pair<std::string, ValueType>> parseParameterList();
        std::unique_ptr<ASTNode> tryParseForEach(); // Returns nullptr if not for-each pattern
    };
}

