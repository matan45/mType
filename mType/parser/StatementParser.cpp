#include "StatementParser.hpp"
#include "Parser.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/statements/IfNode.hpp"
#include "../ast/nodes/statements/WhileNode.hpp"
#include "../ast/nodes/statements/BreakNode.hpp"
#include "../ast/nodes/statements/ContinueNode.hpp"
#include "../ast/nodes/functions/ReturnNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"

namespace parser
{
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;
    using namespace token;

    std::unique_ptr<ASTNode> StatementParser::parseStatement()
    {
        const Token& currentToken = parser.getCurrentToken();
        
        switch (currentToken.type) {
            case TokenType::NAMESPACE:
                return parser.getNamespaceParser()->parseNamespace();
            case TokenType::CLASS:
                return parser.getClassParser()->parseClass();
            case TokenType::FUNCTION:
                return parseFunction();
            case TokenType::INT:
            case TokenType::FLOAT:
            case TokenType::BOOL:
            case TokenType::STRING_TYPE:
                return parseDeclaration();
            case TokenType::FINAL:
            case TokenType::STATIC:
                return parseDeclaration(); // Handle modifiers in parseDeclaration
            case TokenType::IF:
                return parseIfStatement();
            case TokenType::WHILE:
                return parseWhileStatement();
            case TokenType::DO:
                return parseDoWhileStatement();
            case TokenType::FOR:
                return parseForStatement();
            case TokenType::SWITCH:
                return parseSwitchStatement();
            case TokenType::BREAK:
                return parseBreakStatement();
            case TokenType::CONTINUE:
                return parseContinueStatement();
            case TokenType::RETURN:
                return parseReturnStatement();
            case TokenType::LBRACE:
                return parseBlock();
            case TokenType::IDENTIFIER:
                return parseAssignment(); // Could be assignment or function call
            case TokenType::SEMICOLON:
                parser.advanceToken(); // Skip empty statement
                return nullptr;
            case TokenType::IMPORT:
                return parseImport();
            default:
                return parseExpressionStatement();
        }
    }

    std::unique_ptr<ASTNode> StatementParser::parseBlock()
    {
        parser.expectToken(TokenType::LBRACE);
        auto block = std::make_unique<BlockNode>();
        
        while (parser.getCurrentToken().type != TokenType::RBRACE && parser.getCurrentToken().type != TokenType::END) {
            auto statement = parseStatement();
            if (statement) {
                block->addStatement(std::move(statement));
            }
        }
        
        parser.expectToken(TokenType::RBRACE);
        return std::move(block);
    }

    std::unique_ptr<ASTNode> StatementParser::parseDeclaration()
    {
        bool isFinal = false;
        bool isStatic = false;
        
        // Handle modifiers
        if (parser.getCurrentToken().type == TokenType::FINAL) {
            isFinal = true;
            parser.advanceToken();
        }
        if (parser.getCurrentToken().type == TokenType::STATIC) {
            isStatic = true;
            parser.advanceToken();
        }
        
        ValueType type = parseType();
        
        if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected variable name");
        }
        
        std::string varName = parser.getCurrentToken().stringValue;
        parser.advanceToken();
        
        std::unique_ptr<ASTNode> value = nullptr;
        if (parser.matchToken(TokenType::ASSIGN)) {
            value = parser.parseExpression();
        }
        
        parser.expectToken(TokenType::SEMICOLON);
        
        return std::make_unique<AssignmentNode>(varName, std::move(value), type, true, isFinal, isStatic);
    }

    std::unique_ptr<ASTNode> StatementParser::parseAssignment()
    {
        std::string varName = parser.getCurrentToken().stringValue;
        parser.advanceToken();
        
        if (parser.matchToken(TokenType::ASSIGN)) {
            auto value = parser.parseExpression();
            parser.expectToken(TokenType::SEMICOLON);
            return std::make_unique<AssignmentNode>(varName, std::move(value));
        } else {
            // It's a function call or expression statement
            // Put the identifier back for expression parsing
            // This is a simplified approach - in a real parser you'd look ahead
            return parseExpressionStatement();
        }
    }

    std::unique_ptr<ASTNode> StatementParser::parseIfStatement()
    {
        parser.expectToken(TokenType::IF);
        parser.expectToken(TokenType::LPAREN);
        auto condition = parser.parseExpression();
        parser.expectToken(TokenType::RPAREN);
        
        auto thenStatement = parseStatement();
        
        std::unique_ptr<ASTNode> elseStatement = nullptr;
        if (parser.matchToken(TokenType::ELSE)) {
            elseStatement = parseStatement();
        }
        
        return std::make_unique<IfNode>(std::move(condition), std::move(thenStatement), std::move(elseStatement));
    }

    std::unique_ptr<ASTNode> StatementParser::parseWhileStatement()
    {
        parser.expectToken(TokenType::WHILE);
        parser.expectToken(TokenType::LPAREN);
        auto condition = parser.parseExpression();
        parser.expectToken(TokenType::RPAREN);
        
        auto body = parseStatement();
        
        return std::make_unique<WhileNode>(std::move(condition), std::move(body));
    }

    std::unique_ptr<ASTNode> StatementParser::parseDoWhileStatement()
    {
        // Simplified implementation
        parser.advanceToken(); // Skip 'do'
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseForStatement()
    {
        // Simplified implementation
        parser.advanceToken(); // Skip 'for'
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseSwitchStatement()
    {
        // Simplified implementation
        parser.advanceToken(); // Skip 'switch'
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseBreakStatement()
    {
        parser.advanceToken(); // Skip 'break'
        parser.expectToken(TokenType::SEMICOLON);
        return std::make_unique<BreakNode>();
    }

    std::unique_ptr<ASTNode> StatementParser::parseContinueStatement()
    {
        parser.advanceToken(); // Skip 'continue'
        parser.expectToken(TokenType::SEMICOLON);
        return std::make_unique<ContinueNode>();
    }

    std::unique_ptr<ASTNode> StatementParser::parseReturnStatement()
    {
        parser.advanceToken(); // Skip 'return'
        
        std::unique_ptr<ASTNode> value = nullptr;
        if (parser.getCurrentToken().type != TokenType::SEMICOLON) {
            value = parser.parseExpression();
        }
        
        parser.expectToken(TokenType::SEMICOLON);
        return std::make_unique<ReturnNode>(std::move(value));
    }

    std::unique_ptr<ASTNode> StatementParser::parseExpressionStatement()
    {
        auto expr = parser.parseExpression();
        parser.expectToken(TokenType::SEMICOLON);
        return expr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseFunction()
    {
        parser.expectToken(TokenType::FUNCTION);
        
        if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected function name");
        }
        
        std::string funcName = parser.getCurrentToken().stringValue;
        parser.advanceToken();
        
        auto parameters = parseParameterList();
        
        parser.expectToken(TokenType::COLON);
        ValueType returnType = parseType();
        
        auto body = parseBlock();
        
        return std::make_unique<FunctionNode>(funcName, returnType, std::move(parameters), std::move(body));
    }

    std::unique_ptr<ASTNode> StatementParser::parseImport()
    {
        // Simplified implementation
        parser.advanceToken(); // Skip 'import'
        return nullptr;
    }

    ValueType StatementParser::parseType()
    {
        switch (parser.getCurrentToken().type) {
            case TokenType::INT:
                parser.advanceToken();
                return ValueType::INT;
            case TokenType::FLOAT:
                parser.advanceToken();
                return ValueType::FLOAT;
            case TokenType::BOOL:
                parser.advanceToken();
                return ValueType::BOOL;
            case TokenType::STRING_TYPE:
                parser.advanceToken();
                return ValueType::STRING;
            case TokenType::VOID:
                parser.advanceToken();
                return ValueType::VOID;
            case TokenType::IDENTIFIER:
                // Object type (class name)
                parser.advanceToken();
                return ValueType::OBJECT;
            default:
                throw std::runtime_error("Expected type name");
        }
    }

    std::vector<std::pair<std::string, ValueType>> StatementParser::parseParameterList()
    {
        std::vector<std::pair<std::string, ValueType>> parameters;
        
        parser.expectToken(TokenType::LPAREN);
        
        if (parser.getCurrentToken().type != TokenType::RPAREN) {
            // Parse first parameter
            ValueType type = parseType();
            
            if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
                throw std::runtime_error("Expected parameter name");
            }
            
            std::string name = parser.getCurrentToken().stringValue;
            parser.advanceToken();
            
            parameters.emplace_back(name, type);
            
            // Parse additional parameters
            while (parser.matchToken(TokenType::COMMA)) {
                ValueType paramType = parseType();
                
                if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
                    throw std::runtime_error("Expected parameter name");
                }
                
                std::string paramName = parser.getCurrentToken().stringValue;
                parser.advanceToken();
                
                parameters.emplace_back(paramName, paramType);
            }
        }
        
        parser.expectToken(TokenType::RPAREN);
        
        return parameters;
    }
}
