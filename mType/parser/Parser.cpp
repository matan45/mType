#include "Parser.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"
#include "../services/ImportManager.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"

namespace parser
{
    using namespace ast::nodes::statements;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace token;
    using namespace errors;

    Parser::Parser(Lexer& lex, std::unique_ptr<services::ImportManager> manager)
        : importManager(std::move(manager)),
          tokenStream(std::make_unique<TokenStream>(lex)),
          context(std::make_unique<ParseContext>(this)),
          statementParser(std::make_unique<StatementParser>(*tokenStream, *context)),
          expressionParser(std::make_unique<ExpressionParser>(*tokenStream, *context)),
          classParser(std::make_unique<ClassParser>(*tokenStream, *context)),
          interfaceParser(std::make_unique<InterfaceParser>(*tokenStream, *context)),
          annotationDeclarationParser(std::make_unique<AnnotationDeclarationParser>(*tokenStream, *context))
    {
        statementParser->setExpressionParser(*expressionParser);
    }

    std::unique_ptr<services::ImportManager> Parser::getImportManager()
    {
        return std::move(importManager);
    }

    std::unique_ptr<ASTNode> Parser::parseProgram()
    {
        auto program = std::make_unique<ProgramNode>(tokenStream->location());

        int iterationCount = 0;
        while (!tokenStream->isAtEnd())
        {
            iterationCount++;

            if (iterationCount > 1000)
            {
                break;
            }

            auto statement = parseStatement();
            if (statement)
            {
                program->addStatement(std::move(statement));
            }
        }

        return program;
    }

    std::unique_ptr<ASTNode> Parser::parseStatement()
    {
        return statementParser->parseStatement();
    }

    std::unique_ptr<ASTNode> Parser::parseExpression()
    {
        return expressionParser->parseExpression();
    }

    std::unique_ptr<ASTNode> Parser::parseClass()
    {
        return classParser->parseClass();
    }

    std::unique_ptr<ASTNode> Parser::parseInterface()
    {
        return interfaceParser->parseInterface();
    }

    std::unique_ptr<ASTNode> Parser::parseAnnotationDeclaration()
    {
        return annotationDeclarationParser->parseAnnotationDeclaration();
    }

    std::unique_ptr<ASTNode> Parser::parseNewExpression()
    {
        return classParser->parseNewExpression();
    }
}
