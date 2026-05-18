#pragma once
#include "../lexer/Lexer.hpp"
#include "../ast/ASTNode.hpp"
#include "../token/TokenType.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"
#include "StatementParser.hpp"
#include "ExpressionParser.hpp"
#include "ClassParser.hpp"
#include "InterfaceParser.hpp"
#include "AnnotationDeclarationParser.hpp"
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

        std::unique_ptr<TokenStream> tokenStream;
        std::unique_ptr<ParseContext> context;

        std::unique_ptr<StatementParser> statementParser;
        std::unique_ptr<ExpressionParser> expressionParser;
        std::unique_ptr<ClassParser> classParser;
        std::unique_ptr<InterfaceParser> interfaceParser;
        std::unique_ptr<AnnotationDeclarationParser> annotationDeclarationParser;

    public:
        explicit Parser(Lexer& lex, std::unique_ptr<services::ImportManager> manager);
        ~Parser() = default;

        std::unique_ptr<services::ImportManager> getImportManager();

        std::unique_ptr<ASTNode> parseProgram();
        std::unique_ptr<ASTNode> parseStatement();
        std::unique_ptr<ASTNode> parseExpression();
        std::unique_ptr<ASTNode> parseClass();
        std::unique_ptr<ASTNode> parseInterface();
        std::unique_ptr<ASTNode> parseAnnotationDeclaration();
        std::unique_ptr<ASTNode> parseNewExpression();
    };
}
