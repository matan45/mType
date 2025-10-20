#pragma once
#include <memory>
#include "../ast/ASTNode.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"
#include "statement/ControlFlowParser.hpp"
#include "statement/LoopParser.hpp"
#include "statement/DeclarationParser.hpp"
#include "statement/AssignmentStatementParser.hpp"
#include "statement/FunctionParser.hpp"
#include "statement/ImportParser.hpp"
#include "statement/ExceptionParser.hpp"
#include "utilities/StatementTypeDetector.hpp"

namespace parser
{
    class ParseContext;
    class ExpressionParser; // Forward declaration
    using namespace ast;
    using namespace parser::statement;
    using namespace parser::utilities;

    class StatementParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;
        ExpressionParser* expressionParser; // Reference to ExpressionParser to break circular dependency

        // Specialized parser helpers
        std::unique_ptr<ControlFlowParser> controlFlowParser;
        std::unique_ptr<LoopParser> loopParser;
        std::unique_ptr<DeclarationParser> declarationParser;
        std::unique_ptr<AssignmentStatementParser> assignmentParser;
        std::unique_ptr<FunctionParser> functionParser;
        std::unique_ptr<ImportParser> importParser;
        std::unique_ptr<ExceptionParser> exceptionParser;

    public:
        explicit StatementParser(TokenStream& stream, ParseContext& ctx);


        // Method to set ExpressionParser reference after construction
        void setExpressionParser(ExpressionParser& exprParser)
        {
            expressionParser = &exprParser;
            // Also set it in the assignment parser to break circular dependency
            if (assignmentParser)
            {
                assignmentParser->setExpressionParser(exprParser);
            }
        }

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
        std::unique_ptr<ASTNode> parseTryStatement();
        std::unique_ptr<ASTNode> parseThrowStatement();

    private:
        void initializeHelperParsers();
        std::unique_ptr<ASTNode> delegateToSpecializedParser(StatementType type);
    };
}
