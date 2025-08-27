#include "Parser.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/expressions/IntegerNode.hpp"
#include "../ast/nodes/expressions/FloatNode.hpp"
#include "../ast/nodes/expressions/StringNode.hpp"
#include "../ast/nodes/expressions/BoolNode.hpp"
#include "../ast/nodes/expressions/NullNode.hpp"
#include "../ast/nodes/expressions/VariableNode.hpp"
#include "../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/functions/FunctionCallNode.hpp"

namespace parser
{
    using namespace ast::nodes::statements;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace token;

    Parser::Parser(Lexer& lex) 
        : lexer(lex), currentToken(lexer.getNextToken())
    {
        // Initialize subparsers with reference to main parser
        statementParser = std::make_unique<StatementParser>(*this);
        expressionParser = std::make_unique<ExpressionParser>(*this);
        namespaceParser = std::make_unique<NamespaceParser>(*this);
        classParser = std::make_unique<ClassParser>(*this);
    }

    std::unique_ptr<ASTNode> Parser::parseProgram()
    {
        auto program = std::make_unique<ProgramNode>(currentToken.location);
        
        while (currentToken.type != TokenType::END)
        {
            try {
                auto statement = parseStatement();
                if (statement) {
                    program->addStatement(std::move(statement));
                }
            }
            catch (const std::exception& e) {
                // Handle parse errors - could log or throw up
                advance(); // Skip problematic token and continue
            }
        }
        
        return std::move(program);
    }

    void Parser::advance()
    {
        currentToken = lexer.getNextToken();
    }

    bool Parser::match(TokenType type)
    {
        if (currentToken.type == type) {
            advance();
            return true;
        }
        return false;
    }

    void Parser::expect(TokenType type)
    {
        if (!match(type)) {
            throw std::runtime_error("Expected token type " + std::to_string(static_cast<int>(type)));
        }
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
