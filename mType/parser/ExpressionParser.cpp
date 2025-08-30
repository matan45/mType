#include "ExpressionParser.hpp"
#include "Parser.hpp"
#include "../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../ast/nodes/expressions/IntegerNode.hpp"
#include "../ast/nodes/expressions/FloatNode.hpp"
#include "../ast/nodes/expressions/StringNode.hpp"
#include "../ast/nodes/expressions/BoolNode.hpp"
#include "../ast/nodes/expressions/NullNode.hpp"
#include "../ast/nodes/expressions/VariableNode.hpp"
#include "../ast/nodes/functions/FunctionCallNode.hpp"
#include "../ast/nodes/classes/MemberAccessNode.hpp"
#include "../ast/nodes/classes/MethodCallNode.hpp"
#include "../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../ast/nodes/namespaces/QualifiedNameNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/statements/QualifiedAssignmentNode.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::classes;
    using namespace token;

    std::unique_ptr<ASTNode> ExpressionParser::parseExpression()
    {
        return parseAssignment();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseAssignment()
    {
        auto expr = parseTernary();

        // Check if this is an assignment expression
        if (parser.getCurrentToken().type == TokenType::ASSIGN ||
            parser.getCurrentToken().type == TokenType::PLUS_ASSIGN ||
            parser.getCurrentToken().type == TokenType::MINUS_ASSIGN ||
            parser.getCurrentToken().type == TokenType::MULTIPLY_ASSIGN ||
            parser.getCurrentToken().type == TokenType::DIVIDE_ASSIGN ||
            parser.getCurrentToken().type == TokenType::MODULO_ASSIGN)
        {
            // For assignment expressions, the left side should be a variable, member access, or qualified name
            auto variableNode = dynamic_cast<VariableNode*>(expr.get());
            auto memberAccessNode = dynamic_cast<MemberAccessNode*>(expr.get());
            auto qualifiedNameNode = dynamic_cast<ast::nodes::namespaces::QualifiedNameNode*>(expr.get());
            if (!variableNode && !memberAccessNode && !qualifiedNameNode) {
                throw ParseException("Invalid assignment target", parser.getCurrentToken().location);
            }

            TokenType opType = parser.getCurrentToken().type;
            parser.advanceToken();
            auto rightExpr = parseAssignment(); // Right associative

            // Create appropriate assignment node based on target type and operator
            if (memberAccessNode) {
                // Member assignment (e.g., car.year = 2023)
                if (opType == TokenType::ASSIGN) {
                    return std::make_unique<MemberAssignmentNode>(
                        memberAccessNode->releaseObject(),
                        memberAccessNode->getMemberName(),
                        std::move(rightExpr));
                } else {
                    // Compound member assignment - not fully implemented yet
                    throw ParseException("Compound assignment to member not yet supported", parser.getCurrentToken().location);
                }
            } else if (variableNode) {
                // Variable assignment
                if (opType == TokenType::ASSIGN) {
                    // Simple assignment - create regular AssignmentNode with VOID type (not a declaration)
                    return std::make_unique<AssignmentNode>(variableNode->getName(), 
                                                          std::move(rightExpr), 
                                                          ValueType::VOID, "");
                } else {
                    // Compound assignment - we need to expand this to: var = var op right
                    std::unique_ptr<ASTNode> expandedRight;
                    TokenType binaryOp;
                    
                    switch (opType) {
                        case TokenType::PLUS_ASSIGN: binaryOp = TokenType::PLUS; break;
                        case TokenType::MINUS_ASSIGN: binaryOp = TokenType::MINUS; break;
                        case TokenType::MULTIPLY_ASSIGN: binaryOp = TokenType::MULTIPLY; break;
                        case TokenType::DIVIDE_ASSIGN: binaryOp = TokenType::DIVIDE; break;
                        case TokenType::MODULO_ASSIGN: binaryOp = TokenType::MODULO; break;
                        default: throw ParseException("Invalid compound assignment operator", parser.getCurrentToken().location);
                    }

                    // Create: var op right
                    auto leftVar = std::make_unique<VariableNode>(variableNode->getName());
                    expandedRight = std::make_unique<BinaryExpNode>(std::move(leftVar), binaryOp, std::move(rightExpr));

                    return std::make_unique<AssignmentNode>(variableNode->getName(), 
                                                          std::move(expandedRight), 
                                                          ValueType::VOID, "");
                }
            } else if (qualifiedNameNode) {
                // Qualified assignment (e.g., namespace::variable = value)
                if (opType == TokenType::ASSIGN) {
                    return std::make_unique<QualifiedAssignmentNode>(qualifiedNameNode->getQualifiers(), 
                                                                     std::move(rightExpr));
                } else {
                    // Compound qualified assignment - not yet supported
                    throw ParseException("Compound assignment to qualified name not yet supported", parser.getCurrentToken().location);
                }
            }
        }

        return expr;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseTernary()
    {
        auto expr = parseLogicalOr();

        if (parser.matchToken(TokenType::QUESTION))
        {
            auto trueExpr = parseExpression();
            parser.expectToken(TokenType::COLON);
            auto falseExpr = parseExpression();
            return std::make_unique<TernaryExpNode>(std::move(expr), std::move(trueExpr), std::move(falseExpr));
        }

        return expr;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseLogicalOr()
    {
        auto left = parseLogicalAnd();

        while (parser.getCurrentToken().type == TokenType::OR)
        {
            TokenType op = parser.getCurrentToken().type;
            parser.advanceToken();
            auto right = parseLogicalAnd();
            left = std::make_unique<BinaryExpNode>(std::move(left), op, std::move(right));
        }

        return left;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseLogicalAnd()
    {
        auto left = parseEquality();

        while (parser.getCurrentToken().type == TokenType::AND)
        {
            TokenType op = parser.getCurrentToken().type;
            parser.advanceToken();
            auto right = parseEquality();
            left = std::make_unique<BinaryExpNode>(std::move(left), op, std::move(right));
        }

        return left;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseEquality()
    {
        auto left = parseComparison();

        while (parser.getCurrentToken().type == TokenType::EQUALS || parser.getCurrentToken().type ==
            TokenType::NOT_EQUALS)
        {
            TokenType op = parser.getCurrentToken().type;
            parser.advanceToken();
            auto right = parseComparison();
            left = std::make_unique<BinaryExpNode>(std::move(left), op, std::move(right));
        }

        return left;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseComparison()
    {
        auto left = parseAdditive();

        while (parser.getCurrentToken().type == TokenType::LESS || parser.getCurrentToken().type ==
            TokenType::LESS_EQUALS ||
            parser.getCurrentToken().type == TokenType::GREATER || parser.getCurrentToken().type ==
            TokenType::GREATER_EQUALS)
        {
            TokenType op = parser.getCurrentToken().type;
            parser.advanceToken();
            auto right = parseAdditive();
            left = std::make_unique<BinaryExpNode>(std::move(left), op, std::move(right));
        }

        return left;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseAdditive()
    {
        auto left = parseMultiplicative();

        while (parser.getCurrentToken().type == TokenType::PLUS || parser.getCurrentToken().type == TokenType::MINUS)
        {
            TokenType op = parser.getCurrentToken().type;
            parser.advanceToken();
            auto right = parseMultiplicative();
            left = std::make_unique<BinaryExpNode>(std::move(left), op, std::move(right));
        }

        return left;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseMultiplicative()
    {
        auto left = parseUnary();

        while (parser.getCurrentToken().type == TokenType::MULTIPLY || parser.getCurrentToken().type ==
            TokenType::DIVIDE ||
            parser.getCurrentToken().type == TokenType::MODULO)
        {
            TokenType op = parser.getCurrentToken().type;
            parser.advanceToken();
            auto right = parseUnary();
            left = std::make_unique<BinaryExpNode>(std::move(left), op, std::move(right));
        }

        return left;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseUnary()
    {
        // Handle prefix unary operators like !, -, +, ++, --
        if (parser.getCurrentToken().type == TokenType::NOT ||
            parser.getCurrentToken().type == TokenType::MINUS ||
            parser.getCurrentToken().type == TokenType::PLUS ||
            parser.getCurrentToken().type == TokenType::INCREMENT ||
            parser.getCurrentToken().type == TokenType::DECREMENT)
        {
            TokenType op = parser.getCurrentToken().type;
            parser.advanceToken();
            auto operand = parseUnary();
            return std::make_unique<UnaryExpNode>(op, std::move(operand), ast::nodes::expressions::UnaryPosition::PREFIX);
        }

        return parsePostfix();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parsePostfix()
    {
        auto expr = parsePrimary();

        while (true)
        {
            if (parser.getCurrentToken().type == TokenType::INCREMENT ||
                parser.getCurrentToken().type == TokenType::DECREMENT)
            {
                // Postfix increment/decrement
                TokenType op = parser.getCurrentToken().type;
                parser.advanceToken();
                // Create a postfix operation using UnaryExpNode with POSTFIX position
                expr = std::make_unique<UnaryExpNode>(op, std::move(expr), ast::nodes::expressions::UnaryPosition::POSTFIX);
            }
            else if (parser.getCurrentToken().type == TokenType::LPAREN)
            {
                // Function call
                std::string funcName;
                bool canCallFunction = false;

                if (auto varNode = dynamic_cast<VariableNode*>(expr.get()))
                {
                    funcName = varNode->getName();
                    canCallFunction = true;
                }
                else if (auto qualNode = dynamic_cast<ast::nodes::namespaces::QualifiedNameNode*>(expr.get()))
                {
                    funcName = qualNode->getFullName();
                    canCallFunction = true;
                }

                if (canCallFunction)
                {
                    parser.advanceToken(); // Skip '('
                    auto arguments = parseArguments();
                    parser.expectToken(TokenType::RPAREN);
                    // No need to call release() - unique_ptr will handle cleanup automatically
                    expr = std::make_unique<FunctionCallNode>(funcName, std::move(arguments));
                }
            }
            else if (parser.getCurrentToken().type == TokenType::DOT)
            {
                // Member access
                expr = parseMemberAccess(std::move(expr));
            }
            else if (parser.getCurrentToken().type == TokenType::SCOPE)
            {
                // Namespace scope resolution
                parser.advanceToken();
                if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected identifier after '::'", parser.getCurrentToken().location);
                }

                if (auto varNode = dynamic_cast<VariableNode*>(expr.get()))
                {
                    std::vector<std::string> parts = {varNode->getName()};

                    // Add the first identifier after ::
                    parts.push_back(parser.getCurrentToken().stringValue);
                    parser.advanceToken();

                    // Continue parsing if there are more :: tokens
                    while (parser.getCurrentToken().type == TokenType::SCOPE)
                    {
                        parser.advanceToken();
                        if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
                        {
                            throw ParseException("Expected identifier after '::'", parser.getCurrentToken().location);
                        }
                        parts.push_back(parser.getCurrentToken().stringValue);
                        parser.advanceToken();
                    }

                    // No need to call release() - unique_ptr will handle cleanup automatically
                    expr = std::make_unique<ast::nodes::namespaces::QualifiedNameNode>(parts);
                    
                    // Check if this is a function call (e.g., MathUtils::max(10, 5))
                    if (parser.getCurrentToken().type == TokenType::LPAREN)
                    {
                        // Join the parts to create the full function name
                        std::string fullName = parts[0];
                        for (size_t i = 1; i < parts.size(); i++)
                        {
                            fullName += "::" + parts[i];
                        }
                        
                        // Parse as function call using the same method as normal function calls
                        parser.advanceToken(); // consume '('
                        auto arguments = parseArguments();
                        parser.expectToken(TokenType::RPAREN);
                        
                        expr = std::make_unique<ast::nodes::functions::FunctionCallNode>(fullName, std::move(arguments));
                    }
                }
            }
            else
            {
                break;
            }
        }

        return expr;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parsePrimary()
    {
        switch (parser.getCurrentToken().type)
        {
        case TokenType::INT_NUMBER:
            {
                int value = parser.getCurrentToken().intValue;
                parser.advanceToken();
                return std::make_unique<IntegerNode>(value);
            }
        case TokenType::FLOAT_NUMBER:
            {
                float value = parser.getCurrentToken().floatValue;
                parser.advanceToken();
                return std::make_unique<FloatNode>(value);
            }
        case TokenType::STRING_LITERAL:
            {
                std::string value = parser.getCurrentToken().stringValue;
                parser.advanceToken();
                return std::make_unique<StringNode>(value);
            }
        case TokenType::TRUE:
            {
                parser.advanceToken();
                return std::make_unique<BoolNode>(true);
            }
        case TokenType::FALSE:
            {
                parser.advanceToken();
                return std::make_unique<BoolNode>(false);
            }
        case TokenType::NULL_LITERAL:
            {
                parser.advanceToken();
                return std::make_unique<NullNode>();
            }
        case TokenType::IDENTIFIER:
            {
                std::string name = parser.getCurrentToken().stringValue;
                parser.advanceToken();
                return std::make_unique<VariableNode>(name);
            }
        case TokenType::LPAREN:
            {
                parser.advanceToken();
                auto expr = parseExpression();
                parser.expectToken(TokenType::RPAREN);
                return expr;
            }
        case TokenType::NEW:
            {
                // Delegate to ClassParser for proper separation of concerns
                return parser.getClassParser()->parseNewExpression();
            }
        default:
            throw ParseException("Unexpected token in primary expression", parser.getCurrentToken().location);
        }
    }

    std::vector<std::unique_ptr<ASTNode>> ExpressionParser::parseArguments()
    {
        std::vector<std::unique_ptr<ASTNode>> arguments;

        if (parser.getCurrentToken().type != TokenType::RPAREN)
        {
            arguments.push_back(parseExpression());

            while (parser.matchToken(TokenType::COMMA))
            {
                arguments.push_back(parseExpression());
            }
        }

        return arguments;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseMemberAccess(std::unique_ptr<ASTNode> object)
    {
        parser.expectToken(TokenType::DOT);

        if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected member name after '.'", parser.getCurrentToken().location);
        }

        std::string memberName = parser.getCurrentToken().stringValue;
        parser.advanceToken();

        // Check if it's a method call
        if (parser.getCurrentToken().type == TokenType::LPAREN)
        {
            parser.advanceToken();
            auto arguments = parseArguments();
            parser.expectToken(TokenType::RPAREN);
            return std::make_unique<MethodCallNode>(std::move(object), memberName, std::move(arguments));
        }
        else
        {
            return std::make_unique<MemberAccessNode>(std::move(object), memberName);
        }
    }
}
