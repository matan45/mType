#include "Parser.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"
#include "../services/ImportManager.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/expressions/StringNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace ast::nodes::statements;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace token;
    using namespace errors;

    Parser::Parser(Lexer& lex, std::unique_ptr<services::ImportManager> manager)
        : importManager(std::move(manager))
    {
        // Initialize new architecture components
        tokenStream = std::make_unique<TokenStream>(lex);
        
        // Initialize context first with nullptr (will be set later)
        context = std::make_unique<ParseContext>(nullptr, nullptr, nullptr, tokenStream.get());
        
        // Initialize subparsers with new constructor signatures
        statementParser = std::make_unique<StatementParser>(*tokenStream, *context);
        expressionParser = std::make_unique<ExpressionParser>(*tokenStream, *context);
        classParser = std::make_unique<ClassParser>(*tokenStream, *context);
        
        // Set subparser references in context
        context->setStatementParser(statementParser.get());
        context->setExpressionParser(expressionParser.get());
        context->setClassParser(classParser.get());
    }

    std::unique_ptr<ASTNode> Parser::parseProgram()
    {
        auto program = std::make_unique<ProgramNode>(tokenStream->location());

        while (!tokenStream->isAtEnd())
        {
            auto statement = parseStatement();
            if (statement)
            {
                // All statements, including import nodes, are added to the program
                // Import processing is completely deferred to evaluation phase
                program->addStatement(std::move(statement));
            }
        }

        return program;
    }


    std::unique_ptr<ASTNode> Parser::parseStatement()
    {
        // Delegate to StatementParser
        return statementParser->parseStatement();
    }

    std::unique_ptr<ASTNode> Parser::parseExpression()
    {
        // Delegate to ExpressionParser
        return expressionParser->parseExpression();
    }

}
