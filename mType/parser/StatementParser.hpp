#pragma once
#include <memory>
#include <string>
#include <vector>
#include "../ast/ASTNode.hpp"
#include "../ast/nodes/statements/TryNode.hpp"
#include "../ast/nodes/statements/CatchNode.hpp"
#include "../ast/nodes/statements/ThrowNode.hpp"
#include "../errors/SourceLocation.hpp"
#include "../token/Token.hpp"
#include "../token/TokenType.hpp"
#include "../value/ValueType.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"
#include "utilities/StatementTypeDetector.hpp"

namespace parser
{
    class ExpressionParser;
    using namespace ast;
    using namespace parser::utilities;

    class StatementParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;
        ExpressionParser* expressionParser;

        void expectToken(token::TokenType type);
        bool tryConsumeToken(token::TokenType type);
        const token::Token& currentToken() const;
        bool isAtEnd() const;

    public:
        explicit StatementParser(TokenStream& stream, ParseContext& ctx);

        void setExpressionParser(ExpressionParser& exprParser) { expressionParser = &exprParser; }

        std::unique_ptr<ASTNode> parseStatement();
        std::unique_ptr<ASTNode> parseBlock();

    private:
        std::unique_ptr<ASTNode> delegateToSpecializedParser(StatementType type);

        // Control flow (absorbed from ControlFlowParser)
        std::unique_ptr<ASTNode> parseIfStatement();
        std::unique_ptr<ASTNode> parseSwitchStatement();
        std::unique_ptr<ASTNode> parseMatchStatement();
        std::unique_ptr<ASTNode> parseBreakStatement();
        std::unique_ptr<ASTNode> parseContinueStatement();
        std::unique_ptr<ASTNode> parseReturnStatement();
        bool isTypePatternStart() const;

        // Loops (absorbed from LoopParser)
        std::unique_ptr<ASTNode> parseWhileStatement();
        std::unique_ptr<ASTNode> parseDoWhileStatement();
        std::unique_ptr<ASTNode> parseForStatement();

        // Declarations (absorbed from DeclarationParser)
        std::unique_ptr<ASTNode> parseDeclaration();
        struct ModifierInfo { bool isFinal = false; bool isStatic = false; };
        ModifierInfo parseModifiers();
        std::unique_ptr<ASTNode> applyAutoBoxingIfNeeded(std::unique_ptr<ASTNode> value,
                                                         const std::string& targetClassName,
                                                         value::ValueType targetType);

        // Assignment (absorbed from AssignmentStatementParser)
        std::unique_ptr<ASTNode> parseAssignment();
        std::unique_ptr<ASTNode> parseExpressionStatement();
        bool isAssignmentOperator(token::TokenType type) const noexcept;
        token::TokenType getCorrespondingBinaryOperator(token::TokenType assignmentOp) const;
        std::unique_ptr<ASTNode> createCompoundAssignment(const std::string& varName,
                                                          const errors::SourceLocation& location,
                                                          token::TokenType opType,
                                                          std::unique_ptr<ASTNode> value);

        // Function (absorbed from FunctionParser)
        std::unique_ptr<ASTNode> parseFunction();
        void validateFunctionName(const std::string& funcName);

        // Import (absorbed from ImportParser)
        std::unique_ptr<ASTNode> parseImport();
        std::unique_ptr<ASTNode> parseSelectiveImport(const errors::SourceLocation& loc);
        std::unique_ptr<ASTNode> parseWildcardImport(const errors::SourceLocation& loc);
        std::unique_ptr<ASTNode> parseLibraryImport(const errors::SourceLocation& loc);
        void validateImportPath(const std::string& path);

        // Exception (absorbed from ExceptionParser)
        std::unique_ptr<ast::nodes::statements::TryNode> parseTryStatement();
        std::unique_ptr<ast::nodes::statements::ThrowNode> parseThrowStatement();
        std::unique_ptr<ast::nodes::statements::CatchNode> parseCatchClause();
        std::unique_ptr<ASTNode> parseFinallyClause();
    };
}
