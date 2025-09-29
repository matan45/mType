#pragma once
#include <memory>
#include <vector>
#include "../ast/ASTNode.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"
#include "error/ErrorHandler.hpp"
#include "statement/ControlFlowParser.hpp"
#include "statement/LoopParser.hpp"
#include "statement/DeclarationParser.hpp"
#include "statement/AssignmentStatementParser.hpp"
#include "statement/FunctionParser.hpp"
#include "statement/ImportParser.hpp"
#include "utilities/StatementTypeDetector.hpp"

namespace parser
{
    class ParseContext;
    using namespace ast;
    using namespace parser::statement;
    using namespace parser::utilities;

    class StatementParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;
        std::shared_ptr<error::ErrorHandler> errorHandler;

        // Specialized parser helpers
        std::unique_ptr<ControlFlowParser> controlFlowParser;
        std::unique_ptr<LoopParser> loopParser;
        std::unique_ptr<DeclarationParser> declarationParser;
        std::unique_ptr<AssignmentStatementParser> assignmentParser;
        std::unique_ptr<FunctionParser> functionParser;
        std::unique_ptr<ImportParser> importParser;

    public:
        explicit StatementParser(TokenStream& stream, ParseContext& ctx)
            : tokenStream(stream), context(ctx), errorHandler(std::make_shared<error::ErrorHandler>())
        {
            initializeHelperParsers();
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

    private:
        void initializeHelperParsers();
        std::unique_ptr<ASTNode> delegateToSpecializedParser(StatementType type);

        // Helper methods
        std::vector<std::pair<std::string, ValueType>> parseParameterList();
        std::unique_ptr<ASTNode> tryParseForEach(); // Returns nullptr if not for-each pattern
    };
}

