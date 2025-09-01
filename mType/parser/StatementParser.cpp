#include "StatementParser.hpp"
#include "TypeParser.hpp"
#include "ParserUtils.hpp"
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
#include "../ast/nodes/functions/FunctionCallNode.hpp"
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
        const Token& currentToken = tokenStream.current();

        switch (currentToken.type)
        {
        case TokenType::CLASS:
            return context.parseClass();
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
                Token nextToken = tokenStream.peek();
                if (nextToken.type == TokenType::IDENTIFIER) {
                    // Pattern: "ClassName varName" - this is a custom type declaration
                    return parseDeclaration();
                } else if (nextToken.type == TokenType::SCOPE) {
                    // Pattern: "identifier::..." - this is always an expression (qualified call/assignment/access)
                    // Static method calls, static field access, static field assignment, etc.
                    return parseExpressionStatement();
                } else if (TypeParser::isAssignmentOperator(nextToken.type)) {
                    // Pattern: "varName =" - this is an assignment
                    return parseAssignment();
                } else {
                    // Pattern: "identifier.method()" or "functionCall()" - this is an expression statement
                    return parseExpressionStatement();
                }
            }
        case TokenType::SEMICOLON:
            tokenStream.advance(); // Skip empty statement
            return nullptr;
        case TokenType::IMPORT:
            return parseImport();
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
        tokenStream.expect(TokenType::LBRACE);
        auto block = std::make_unique<BlockNode>();

        while (!tokenStream.check(TokenType::RBRACE) && !tokenStream.isAtEnd())
        {
            auto statement = parseStatement();
            if (statement)
            {
                block->addStatement(std::move(statement));
            }
        }

        tokenStream.expect(TokenType::RBRACE);
        return std::move(block);
    }

    std::unique_ptr<ASTNode> StatementParser::parseDeclaration()
    {
        bool isFinal = false;
        bool isStatic = false;

        // Handle modifiers
        if (tokenStream.current().type == TokenType::FINAL)
        {
            isFinal = true;
            tokenStream.advance();
        }
        if (tokenStream.current().type == TokenType::STATIC)
        {
            isStatic = true;
            tokenStream.advance();
        }

        auto [type, className] = TypeParser::parseTypeWithClassName(tokenStream);

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            // Special case: if we see a parenthesis after a qualified name,
            // it's likely a static method call that was mistakenly routed here
            if (tokenStream.current().type == TokenType::LPAREN && !className.empty() && className.find("::") != std::string::npos) {
                // This is actually a static method call (e.g., Class::method())
                // Parse it as an expression statement instead of a declaration
                
                // Reset the parser position and parse as expression
                // We need to backtrack and reparse this as an expression
                // The className contains the full qualified name like "BankAccount::setInterestRate"
                
                // Parse arguments
                tokenStream.advance(); // consume '('
                std::vector<std::unique_ptr<ASTNode>> arguments;
                if (!tokenStream.check(TokenType::RPAREN))
                {
                    arguments.push_back(context.parseExpression());
                    while (tokenStream.match(TokenType::COMMA))
                    {
                        arguments.push_back(context.parseExpression());
                    }
                }
                tokenStream.expect(TokenType::RPAREN);
                tokenStream.expect(TokenType::SEMICOLON);
                
                // Create a FunctionCallNode with the qualified name
                return std::make_unique<ast::nodes::functions::FunctionCallNode>(className, std::move(arguments));
            }
            throw ParseException("Expected variable name", tokenStream.current().location);
        }

        std::string varName = tokenStream.current().stringValue;
        tokenStream.advance();

        std::unique_ptr<ASTNode> value = nullptr;
        if (tokenStream.match(TokenType::ASSIGN))
        {
            value = context.parseExpression();
        }

        tokenStream.expect(TokenType::SEMICOLON);

        return std::make_unique<AssignmentNode>(varName, std::move(value), type, className, isFinal, isStatic);
    }

    std::unique_ptr<ASTNode> StatementParser::parseAssignment()
    {
        std::string varName = tokenStream.current().stringValue;
        tokenStream.advance();

        // Check for compound assignment operators
        TokenType opType = tokenStream.current().type;
        if (opType == TokenType::ASSIGN ||
            opType == TokenType::PLUS_ASSIGN ||
            opType == TokenType::MINUS_ASSIGN ||
            opType == TokenType::MULTIPLY_ASSIGN ||
            opType == TokenType::DIVIDE_ASSIGN ||
            opType == TokenType::MODULO_ASSIGN)
        {
            tokenStream.advance();
            auto value = context.parseExpression();

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

            tokenStream.expect(TokenType::SEMICOLON);
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
        tokenStream.expect(TokenType::IF);
        tokenStream.expect(TokenType::LPAREN);
        auto condition = context.parseExpression();
        tokenStream.expect(TokenType::RPAREN);

        auto thenStatement = parseStatement();

        std::unique_ptr<ASTNode> elseStatement = nullptr;
        if (tokenStream.match(TokenType::ELSE))
        {
            elseStatement = parseStatement();
        }

        return std::make_unique<IfNode>(std::move(condition), std::move(thenStatement), std::move(elseStatement));
    }

    std::unique_ptr<ASTNode> StatementParser::parseWhileStatement()
    {
        tokenStream.expect(TokenType::WHILE);
        tokenStream.expect(TokenType::LPAREN);
        auto condition = context.parseExpression();
        tokenStream.expect(TokenType::RPAREN);

        auto body = parseStatement();

        return std::make_unique<WhileNode>(std::move(condition), std::move(body));
    }

    std::unique_ptr<ASTNode> StatementParser::parseDoWhileStatement()
    {
        tokenStream.expect(TokenType::DO);
        auto body = parseStatement();
        tokenStream.expect(TokenType::WHILE);
        tokenStream.expect(TokenType::LPAREN);
        auto condition = context.parseExpression();
        tokenStream.expect(TokenType::RPAREN);
        tokenStream.expect(TokenType::SEMICOLON);

        return std::make_unique<DoWhileNode>(std::move(body), std::move(condition));
    }

    std::unique_ptr<ASTNode> StatementParser::parseForStatement()
    {
        tokenStream.expect(TokenType::FOR);
        tokenStream.expect(TokenType::LPAREN);

        // Parse initialization
        std::unique_ptr<ASTNode> init = nullptr;
        if (tokenStream.current().type != TokenType::SEMICOLON)
        {
            if (tokenStream.current().type == TokenType::INT ||
                tokenStream.current().type == TokenType::FLOAT ||
                tokenStream.current().type == TokenType::BOOL ||
                tokenStream.current().type == TokenType::STRING_TYPE)
            {
                // Parse declaration inline without consuming semicolon (for loop specific)
                ValueType type = TypeParser::parseType(tokenStream);
                
                if (tokenStream.current().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected variable name", tokenStream.current().location);
                }
                
                std::string varName = tokenStream.current().stringValue;
                tokenStream.advance();
                
                std::unique_ptr<ASTNode> value = nullptr;
                if (tokenStream.match(TokenType::ASSIGN))
                {
                    value = context.parseExpression();
                }
                
                init = std::make_unique<AssignmentNode>(varName, std::move(value), type, "", false, false);
            }
            else
            {
                init = context.parseExpression();
            }
        }
        
        tokenStream.expect(TokenType::SEMICOLON);

        // Parse condition
        std::unique_ptr<ASTNode> condition = nullptr;
        if (tokenStream.current().type != TokenType::SEMICOLON)
        {
            condition = context.parseExpression();
        }
        tokenStream.expect(TokenType::SEMICOLON);

        // Parse update
        std::unique_ptr<ASTNode> update = nullptr;
        if (tokenStream.current().type != TokenType::RPAREN)
        {
            update = context.parseExpression();
        }
        tokenStream.expect(TokenType::RPAREN);

        auto body = parseStatement();

        return std::make_unique<ForNode>(std::move(init), std::move(condition),
                                         std::move(update), std::move(body));
    }

    std::unique_ptr<ASTNode> StatementParser::parseSwitchStatement()
    {
        tokenStream.expect(TokenType::SWITCH);
        tokenStream.expect(TokenType::LPAREN);
        auto expression = context.parseExpression();
        tokenStream.expect(TokenType::RPAREN);
        tokenStream.expect(TokenType::LBRACE);

        auto switchNode = std::make_unique<SwitchNode>(std::move(expression));

        while (!tokenStream.check(TokenType::RBRACE) &&
            tokenStream.current().type != TokenType::END)
        {
            if (tokenStream.current().type == TokenType::CASE)
            {
                tokenStream.advance();
                auto caseValue = context.parseExpression();
                tokenStream.expect(TokenType::COLON);

                auto caseNode = std::make_unique<CaseNode>(std::move(caseValue));
                while (!tokenStream.check(TokenType::CASE) &&
                    tokenStream.current().type != TokenType::DEFAULT &&
                    !tokenStream.check(TokenType::RBRACE) &&
                    tokenStream.current().type != TokenType::END)
                {
                    auto stmt = parseStatement();
                    if (stmt)
                    {
                        caseNode->addStatement(std::move(stmt));
                    }
                }

                switchNode->addCase(std::move(caseNode));
            }
            else if (tokenStream.current().type == TokenType::DEFAULT)
            {
                tokenStream.advance();
                tokenStream.expect(TokenType::COLON);

                auto defaultNode = std::make_unique<DefaultCaseNode>();
                while (!tokenStream.check(TokenType::CASE) &&
                    tokenStream.current().type != TokenType::DEFAULT &&
                    !tokenStream.check(TokenType::RBRACE) &&
                    tokenStream.current().type != TokenType::END)
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
                tokenStream.advance();
            }
        }

        tokenStream.expect(TokenType::RBRACE);

        return std::move(switchNode);
    }

    std::unique_ptr<ASTNode> StatementParser::parseBreakStatement()
    {
        tokenStream.advance(); // Skip 'break'
        tokenStream.expect(TokenType::SEMICOLON);
        return std::make_unique<BreakNode>();
    }

    std::unique_ptr<ASTNode> StatementParser::parseContinueStatement()
    {
        tokenStream.advance(); // Skip 'continue'
        tokenStream.expect(TokenType::SEMICOLON);
        return std::make_unique<ContinueNode>();
    }

    std::unique_ptr<ASTNode> StatementParser::parseReturnStatement()
    {
        tokenStream.advance(); // Skip 'return'

        std::unique_ptr<ASTNode> value = nullptr;
        if (tokenStream.current().type != TokenType::SEMICOLON)
        {
            value = context.parseExpression();
        }

        tokenStream.expect(TokenType::SEMICOLON);
        return std::make_unique<ReturnNode>(std::move(value));
    }

    std::unique_ptr<ASTNode> StatementParser::parseExpressionStatement()
    {
        auto expr = context.parseExpression();
        
        // Check if this is a member assignment (obj.field = value)
        if (auto memberAccess = dynamic_cast<ast::nodes::classes::MemberAccessNode*>(expr.get())) {
            TokenType opType = tokenStream.current().type;
            if (TypeParser::isAssignmentOperator(opType)) {
                
                tokenStream.advance(); // consume assignment operator
                auto value = context.parseExpression();
                
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
                    tokenStream.expect(TokenType::SEMICOLON);
                    return value;
                }
                
                tokenStream.expect(TokenType::SEMICOLON);
                
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
        
        tokenStream.expect(TokenType::SEMICOLON);
        return expr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseFunction()
    {
        bool isNative = false;

        // Check for native keyword
        if (tokenStream.current().type == TokenType::NATIVE)
        {
            isNative = true;
            tokenStream.advance();
        }

        tokenStream.expect(TokenType::FUNCTION);

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected function name", tokenStream.current().location);
        }

        std::string funcName = tokenStream.current().stringValue;
        tokenStream.advance();

        auto parameters = parseParameterList();

        tokenStream.expect(TokenType::COLON);
        ValueType returnType = TypeParser::parseType(tokenStream);

        std::unique_ptr<ASTNode> body = nullptr;

        if (isNative)
        {
            // Native functions don't have a body
            tokenStream.expect(TokenType::SEMICOLON);
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
        tokenStream.expect(TokenType::IMPORT);

        if (tokenStream.current().type != TokenType::STRING_LITERAL)
        {
            throw ParseException("Expected string literal after 'import'", tokenStream.current().location);
        }

        std::string filePath = tokenStream.current().stringValue;
        SourceLocation loc = tokenStream.current().location;
        tokenStream.advance();

        tokenStream.expect(TokenType::SEMICOLON);

        // Create a pure import node - no processing, no ImportManager dependency
        auto importNode = std::make_unique<ImportNode>(filePath, loc);

        // No processing at parse time - completely defer to evaluation phase

        return importNode;
    }


    std::vector<std::pair<std::string, ValueType>> StatementParser::parseParameterList()
    {
        // Delegate to centralized utility
        return ParserUtils::parseParameterList(tokenStream, true);
    }

    std::unique_ptr<ASTNode> StatementParser::parseNativeFunction()
    {
        tokenStream.expect(TokenType::NATIVE);
        tokenStream.expect(TokenType::FUNCTION);

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected function name", tokenStream.current().location);
        }

        std::string funcName = tokenStream.current().stringValue;
        tokenStream.advance();

        auto parameters = parseParameterList();

        tokenStream.expect(TokenType::COLON);
        ValueType returnType = TypeParser::parseType(tokenStream);

        tokenStream.expect(TokenType::SEMICOLON);

        // Native functions don't have a body
        return std::make_unique<FunctionNode>(funcName, returnType, std::move(parameters), nullptr);
    }

}
