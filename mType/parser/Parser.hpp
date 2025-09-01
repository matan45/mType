#pragma once
#include "../lexer/Lexer.hpp"
#include "../ast/ASTNode.hpp"
#include "../token/TokenType.hpp"
#include "../value/ValueType.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"
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
        std::unique_ptr<services::ImportManager> importManager;
        
        // New architecture components
        std::unique_ptr<TokenStream> tokenStream;
        std::unique_ptr<ParseContext> context;
        
        // Subparsers
        std::unique_ptr<StatementParser> statementParser;
        std::unique_ptr<ExpressionParser> expressionParser;
        std::unique_ptr<ClassParser> classParser;

    public:
        explicit Parser(Lexer& lex, std::unique_ptr<services::ImportManager> manager);
        ~Parser() = default;

        std::unique_ptr<services::ImportManager> getImportManager() { return std::move(importManager); }

        std::unique_ptr<ASTNode> parseProgram();
        std::unique_ptr<ASTNode> parseStatement();
        std::unique_ptr<ASTNode> parseExpression();

    };
}
