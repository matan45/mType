#include "Parser.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"
#include "../services/ImportManager.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include <iostream>

namespace parser
{
    using namespace ast::nodes::statements;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace token;
    using namespace errors;

    // Static helper for atomic initialization
    Parser::ParserComponents Parser::createComponents(Lexer& lex)
    {
        ParserComponents components;
        
        // Step 1: Create TokenStream (no dependencies)
        components.tokenStream = std::make_unique<TokenStream>(lex);
        
        // Step 2: Create ParseContext with immediate initialization
        // Note: We'll update the parsers after they're created
        components.context = std::make_unique<ParseContext>();
        
        // Step 3: Create parsers with references to context and tokenStream
        components.statementParser = std::make_unique<StatementParser>(*components.tokenStream, *components.context);
        components.expressionParser = std::make_unique<ExpressionParser>(*components.tokenStream, *components.context);
        components.classParser = std::make_unique<ClassParser>(*components.tokenStream, *components.context);
        
        // Step 4: Atomically set all parser references in context
        components.context->setStatementParser(*components.statementParser);
        components.context->setExpressionParser(*components.expressionParser);
        components.context->setClassParser(*components.classParser);
        components.context->setTokenStream(*components.tokenStream);
        
        return components;
    }

    Parser::Parser(Lexer& lex, std::unique_ptr<services::ImportManager> manager)
        : importManager(std::move(manager))
    {
        // Atomic initialization using factory method
        auto components = createComponents(lex);
        
        // Move all components into member variables atomically
        tokenStream = std::move(components.tokenStream);
        context = std::move(components.context);
        statementParser = std::move(components.statementParser);
        expressionParser = std::move(components.expressionParser);
        classParser = std::move(components.classParser);
        
        // All components are now fully initialized and consistent
    }

    std::unique_ptr<services::ImportManager> Parser::getImportManager()
    {
        return std::move(importManager);
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
