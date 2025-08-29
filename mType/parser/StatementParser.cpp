#include "StatementParser.hpp"
#include "Parser.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../ast/nodes/classes/MemberAccessNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/statements/IfNode.hpp"
#include "../ast/nodes/statements/WhileNode.hpp"
#include "../ast/nodes/statements/BreakNode.hpp"
#include "../ast/nodes/statements/ContinueNode.hpp"
#include "../ast/nodes/functions/ReturnNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"
#include "../ast/nodes/statements/DoWhileNode.hpp"
#include "../ast/nodes/statements/ForNode.hpp"
#include "../ast/nodes/statements/SwitchNode.hpp"
#include "../ast/nodes/statements/CaseNode.hpp"
#include "../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../ast/nodes/expressions/VariableNode.hpp"
#include "../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../services/ImportManager.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;
    using namespace token;
    using namespace errors;

    std::unique_ptr<ASTNode> StatementParser::parseStatement()
    {
        const Token& currentToken = parser.getCurrentToken();

        switch (currentToken.type)
        {
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
            // Check if this is a custom class type declaration, assignment, or expression statement
            {
                Token nextToken = parser.peekNextToken();
                if (nextToken.type == TokenType::IDENTIFIER) {
                    // Pattern: "ClassName varName" - this is a custom type declaration
                    return parseDeclaration();
                } else if (nextToken.type == TokenType::SCOPE) {
                    // Pattern: "identifier::..." - treat as expression statement
                    // This covers qualified function calls, qualified field access, etc.
                    return parseExpressionStatement();
                } else if (Parser::isAssignmentOperator(nextToken.type)) {
                    // Pattern: "varName =" - this is an assignment
                    return parseAssignment();
                } else {
                    // Pattern: "identifier.method()" or "functionCall()" - this is an expression statement
                    return parseExpressionStatement();
                }
            }
        case TokenType::SEMICOLON:
            parser.advanceToken(); // Skip empty statement
            return nullptr;
        case TokenType::IMPORT:
            return parseImport();
        case TokenType::USING:
            return parser.getNamespaceParser()->parseUsing();
        case TokenType::NATIVE:
            return parseNativeFunction();
        case TokenType::END:
            return nullptr;
        default:
            return parseExpressionStatement();
        }
    }

    std::unique_ptr<ASTNode> StatementParser::parseBlock()
    {
        parser.expectToken(TokenType::LBRACE);
        auto block = std::make_unique<BlockNode>();

        while (parser.getCurrentToken().type != TokenType::RBRACE && parser.getCurrentToken().type != TokenType::END)
        {
            auto statement = parseStatement();
            if (statement)
            {
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
        if (parser.getCurrentToken().type == TokenType::FINAL)
        {
            isFinal = true;
            parser.advanceToken();
        }
        if (parser.getCurrentToken().type == TokenType::STATIC)
        {
            isStatic = true;
            parser.advanceToken();
        }

        auto [type, className] = parser.parseTypeWithClassName();

        if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected variable name", parser.getCurrentToken().location);
        }

        std::string varName = parser.getCurrentToken().stringValue;
        parser.advanceToken();

        std::unique_ptr<ASTNode> value = nullptr;
        if (parser.matchToken(TokenType::ASSIGN))
        {
            value = parser.parseExpression();
        }

        parser.expectToken(TokenType::SEMICOLON);

        return std::make_unique<AssignmentNode>(varName, std::move(value), type, className, isFinal, isStatic);
    }

    std::unique_ptr<ASTNode> StatementParser::parseAssignment()
    {
        std::string varName = parser.getCurrentToken().stringValue;
        parser.advanceToken();

        // Check for compound assignment operators
        TokenType opType = parser.getCurrentToken().type;
        if (opType == TokenType::ASSIGN ||
            opType == TokenType::PLUS_ASSIGN ||
            opType == TokenType::MINUS_ASSIGN ||
            opType == TokenType::MULTIPLY_ASSIGN ||
            opType == TokenType::DIVIDE_ASSIGN ||
            opType == TokenType::MODULO_ASSIGN)
        {
            parser.advanceToken();
            auto value = parser.parseExpression();

            // For compound assignments, create a binary expression
            if (opType != TokenType::ASSIGN)
            {
                auto varNode = std::make_unique<ast::nodes::expressions::VariableNode>(varName);
                TokenType binaryOp;
                switch (opType)
                {
                case TokenType::PLUS_ASSIGN: binaryOp = TokenType::PLUS;
                    break;
                case TokenType::MINUS_ASSIGN: binaryOp = TokenType::MINUS;
                    break;
                case TokenType::MULTIPLY_ASSIGN: binaryOp = TokenType::MULTIPLY;
                    break;
                case TokenType::DIVIDE_ASSIGN: binaryOp = TokenType::DIVIDE;
                    break;
                case TokenType::MODULO_ASSIGN: binaryOp = TokenType::MODULO;
                    break;
                default: binaryOp = TokenType::PLUS;
                    break;
                }
                value = std::make_unique<ast::nodes::expressions::BinaryExpNode>(
                    std::move(varNode), binaryOp, std::move(value));
            }

            parser.expectToken(TokenType::SEMICOLON);
            return std::make_unique<AssignmentNode>(varName, std::move(value), ValueType::VOID, "", false, false);
        }
        else
        {
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
        if (parser.matchToken(TokenType::ELSE))
        {
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
        parser.expectToken(TokenType::DO);
        auto body = parseStatement();
        parser.expectToken(TokenType::WHILE);
        parser.expectToken(TokenType::LPAREN);
        auto condition = parser.parseExpression();
        parser.expectToken(TokenType::RPAREN);
        parser.expectToken(TokenType::SEMICOLON);

        return std::make_unique<DoWhileNode>(std::move(body), std::move(condition));
    }

    std::unique_ptr<ASTNode> StatementParser::parseForStatement()
    {
        parser.expectToken(TokenType::FOR);
        parser.expectToken(TokenType::LPAREN);

        // Parse initialization
        std::unique_ptr<ASTNode> init = nullptr;
        if (parser.getCurrentToken().type != TokenType::SEMICOLON)
        {
            if (parser.getCurrentToken().type == TokenType::INT ||
                parser.getCurrentToken().type == TokenType::FLOAT ||
                parser.getCurrentToken().type == TokenType::BOOL ||
                parser.getCurrentToken().type == TokenType::STRING_TYPE)
            {
                // Parse declaration inline without consuming semicolon (for loop specific)
                ValueType type = parseType();
                
                if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected variable name", parser.getCurrentToken().location);
                }
                
                std::string varName = parser.getCurrentToken().stringValue;
                parser.advanceToken();
                
                std::unique_ptr<ASTNode> value = nullptr;
                if (parser.matchToken(TokenType::ASSIGN))
                {
                    value = parser.parseExpression();
                }
                
                init = std::make_unique<AssignmentNode>(varName, std::move(value), type, "", false, false);
            }
            else
            {
                init = parser.parseExpression();
            }
        }
        
        parser.expectToken(TokenType::SEMICOLON);

        // Parse condition
        std::unique_ptr<ASTNode> condition = nullptr;
        if (parser.getCurrentToken().type != TokenType::SEMICOLON)
        {
            condition = parser.parseExpression();
        }
        parser.expectToken(TokenType::SEMICOLON);

        // Parse update
        std::unique_ptr<ASTNode> update = nullptr;
        if (parser.getCurrentToken().type != TokenType::RPAREN)
        {
            update = parser.parseExpression();
        }
        parser.expectToken(TokenType::RPAREN);

        auto body = parseStatement();

        return std::make_unique<ForNode>(std::move(init), std::move(condition),
                                         std::move(update), std::move(body));
    }

    std::unique_ptr<ASTNode> StatementParser::parseSwitchStatement()
    {
        parser.expectToken(TokenType::SWITCH);
        parser.expectToken(TokenType::LPAREN);
        auto expression = parser.parseExpression();
        parser.expectToken(TokenType::RPAREN);
        parser.expectToken(TokenType::LBRACE);

        auto switchNode = std::make_unique<SwitchNode>(std::move(expression));

        while (parser.getCurrentToken().type != TokenType::RBRACE &&
            parser.getCurrentToken().type != TokenType::END)
        {
            if (parser.getCurrentToken().type == TokenType::CASE)
            {
                parser.advanceToken();
                auto caseValue = parser.parseExpression();
                parser.expectToken(TokenType::COLON);

                auto caseNode = std::make_unique<CaseNode>(std::move(caseValue));
                while (parser.getCurrentToken().type != TokenType::CASE &&
                    parser.getCurrentToken().type != TokenType::DEFAULT &&
                    parser.getCurrentToken().type != TokenType::RBRACE &&
                    parser.getCurrentToken().type != TokenType::END)
                {
                    auto stmt = parseStatement();
                    if (stmt)
                    {
                        caseNode->addStatement(std::move(stmt));
                    }
                }

                switchNode->addCase(std::move(caseNode));
            }
            else if (parser.getCurrentToken().type == TokenType::DEFAULT)
            {
                parser.advanceToken();
                parser.expectToken(TokenType::COLON);

                auto defaultNode = std::make_unique<DefaultCaseNode>();
                while (parser.getCurrentToken().type != TokenType::CASE &&
                    parser.getCurrentToken().type != TokenType::DEFAULT &&
                    parser.getCurrentToken().type != TokenType::RBRACE &&
                    parser.getCurrentToken().type != TokenType::END)
                {
                    auto stmt = parseStatement();
                    if (stmt)
                    {
                        defaultNode->addStatement(std::move(stmt));
                    }
                }

                switchNode->addCase(std::move(defaultNode));
            }
            else
            {
                // Skip unexpected tokens
                parser.advanceToken();
            }
        }

        parser.expectToken(TokenType::RBRACE);

        return std::move(switchNode);
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
        if (parser.getCurrentToken().type != TokenType::SEMICOLON)
        {
            value = parser.parseExpression();
        }

        parser.expectToken(TokenType::SEMICOLON);
        return std::make_unique<ReturnNode>(std::move(value));
    }

    std::unique_ptr<ASTNode> StatementParser::parseExpressionStatement()
    {
        auto expr = parser.parseExpression();
        
        // Check if this is a member assignment (obj.field = value)
        if (auto memberAccess = dynamic_cast<ast::nodes::classes::MemberAccessNode*>(expr.get())) {
            TokenType opType = parser.getCurrentToken().type;
            if (Parser::isAssignmentOperator(opType)) {
                
                parser.advanceToken(); // consume assignment operator
                auto value = parser.parseExpression();
                
                // For compound assignments, create a binary expression
                if (opType != TokenType::ASSIGN) {
                    // For compound assignments, we use the whole member access as the left operand
                    TokenType binaryOp;
                    switch (opType) {
                        case TokenType::PLUS_ASSIGN: binaryOp = TokenType::PLUS; break;
                        case TokenType::MINUS_ASSIGN: binaryOp = TokenType::MINUS; break;
                        case TokenType::MULTIPLY_ASSIGN: binaryOp = TokenType::MULTIPLY; break;
                        case TokenType::DIVIDE_ASSIGN: binaryOp = TokenType::DIVIDE; break;
                        case TokenType::MODULO_ASSIGN: binaryOp = TokenType::MODULO; break;
                        default: binaryOp = TokenType::PLUS; break;
                    }
                    value = std::make_unique<ast::nodes::expressions::BinaryExpNode>(
                        std::move(expr), binaryOp, std::move(value));
                    
                    // After moving expr, memberAccess is no longer valid, so we need to handle this differently
                    parser.expectToken(TokenType::SEMICOLON);
                    return value;
                }
                
                parser.expectToken(TokenType::SEMICOLON);
                
                // Extract the object and member name for the assignment
                auto object = memberAccess->releaseObject();
                std::string memberName = memberAccess->getMemberName();
                
                return std::make_unique<ast::nodes::statements::MemberAssignmentNode>(
                    std::move(object), 
                    memberName, 
                    std::move(value)
                );
            }
        }
        
        parser.expectToken(TokenType::SEMICOLON);
        return expr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseFunction()
    {
        bool isNative = false;

        // Check for native keyword
        if (parser.getCurrentToken().type == TokenType::NATIVE)
        {
            isNative = true;
            parser.advanceToken();
        }

        parser.expectToken(TokenType::FUNCTION);

        if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected function name", parser.getCurrentToken().location);
        }

        std::string funcName = parser.getCurrentToken().stringValue;
        parser.advanceToken();

        auto parameters = parseParameterList();

        parser.expectToken(TokenType::COLON);
        ValueType returnType = parseType();

        std::unique_ptr<ASTNode> body = nullptr;

        if (isNative)
        {
            // Native functions don't have a body
            parser.expectToken(TokenType::SEMICOLON);
        }
        else
        {
            body = parseBlock();
        }

        auto funcNode = std::make_unique<FunctionNode>(funcName, returnType, std::move(parameters), std::move(body));
        // Note: Would need to add isNative flag to FunctionNode
        return funcNode;
    }

    std::unique_ptr<ASTNode> StatementParser::parseImport()
    {
        parser.expectToken(TokenType::IMPORT);

        if (parser.getCurrentToken().type != TokenType::STRING_LITERAL)
        {
            throw ParseException("Expected string literal after 'import'", parser.getCurrentToken().location);
        }

        std::string filePath = parser.getCurrentToken().stringValue;
        SourceLocation loc = parser.getCurrentToken().location;
        parser.advanceToken();

        parser.expectToken(TokenType::SEMICOLON);

        // Create a pure import node - no processing, no ImportManager dependency
        auto importNode = std::make_unique<ImportNode>(filePath, loc);

        // No processing at parse time - completely defer to evaluation phase

        return importNode;
    }

    ValueType StatementParser::parseType()
    {
        return parser.parseType();
    }

    std::vector<std::pair<std::string, ValueType>> StatementParser::parseParameterList()
    {
        std::vector<std::pair<std::string, ValueType>> parameters;

        parser.expectToken(TokenType::LPAREN);

        if (parser.getCurrentToken().type != TokenType::RPAREN)
        {
            // Parse first parameter
            ValueType type = parseType();

            if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected parameter name", parser.getCurrentToken().location);
            }

            std::string name = parser.getCurrentToken().stringValue;
            parser.advanceToken();

            parameters.emplace_back(name, type);

            // Parse additional parameters
            while (parser.matchToken(TokenType::COMMA))
            {
                ValueType paramType = parseType();

                if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected parameter name", parser.getCurrentToken().location);
                }

                std::string paramName = parser.getCurrentToken().stringValue;
                parser.advanceToken();

                parameters.emplace_back(paramName, paramType);
            }
        }

        parser.expectToken(TokenType::RPAREN);

        return parameters;
    }

    std::unique_ptr<ASTNode> StatementParser::parseNativeFunction()
    {
        parser.expectToken(TokenType::NATIVE);
        parser.expectToken(TokenType::FUNCTION);

        if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected function name", parser.getCurrentToken().location);
        }

        std::string funcName = parser.getCurrentToken().stringValue;
        parser.advanceToken();

        auto parameters = parseParameterList();

        parser.expectToken(TokenType::COLON);
        ValueType returnType = parseType();

        parser.expectToken(TokenType::SEMICOLON);

        // Native functions don't have a body
        return std::make_unique<FunctionNode>(funcName, returnType, std::move(parameters), nullptr);
    }

    
}
