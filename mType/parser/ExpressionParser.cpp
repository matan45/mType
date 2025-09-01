#include "ExpressionParser.hpp"
#include "TypeParser.hpp"
#include "../services/ImportManager.hpp"
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
#include "../ast/nodes/statements/AssignmentNode.hpp"
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
        if (tokenStream.current().type == TokenType::ASSIGN ||
            tokenStream.current().type == TokenType::PLUS_ASSIGN ||
            tokenStream.current().type == TokenType::MINUS_ASSIGN ||
            tokenStream.current().type == TokenType::MULTIPLY_ASSIGN ||
            tokenStream.current().type == TokenType::DIVIDE_ASSIGN ||
            tokenStream.current().type == TokenType::MODULO_ASSIGN)
        {
            // For assignment expressions, the left side should be a variable, member access, or qualified name
            auto variableNode = dynamic_cast<VariableNode*>(expr.get());
            auto memberAccessNode = dynamic_cast<MemberAccessNode*>(expr.get());
            if (!variableNode && !memberAccessNode)
            {
                throw ParseException("Invalid assignment target", tokenStream.current().location);
            }

            TokenType opType = tokenStream.current().type;
            tokenStream.advance();
            auto rightExpr = parseAssignment(); // Right associative

            // Create appropriate assignment node based on target type and operator
            if (memberAccessNode)
            {
                // Member assignment (e.g., car.year = 2023)
                if (opType == TokenType::ASSIGN)
                {
                    return std::make_unique<MemberAssignmentNode>(
                        memberAccessNode->releaseObject(),
                        memberAccessNode->getMemberName(),
                        std::move(rightExpr));
                }
                else
                {
                    // Compound member assignment - not fully implemented yet
                    throw ParseException("Compound assignment to member not yet supported",
                                         tokenStream.current().location);
                }
            }
            else if (variableNode)
            {
                // Variable assignment
                if (opType == TokenType::ASSIGN)
                {
                    // Simple assignment - create regular AssignmentNode with VOID type (not a declaration)
                    return std::make_unique<AssignmentNode>(variableNode->getName(),
                                                            std::move(rightExpr),
                                                            ValueType::VOID, "");
                }
                else
                {
                    // Compound assignment - we need to expand this to: var = var op right
                    std::unique_ptr<ASTNode> expandedRight;
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
                    default: throw ParseException("Invalid compound assignment operator",
                                                  tokenStream.current().location);
                    }

                    // Create: var op right
                    auto leftVar = std::make_unique<VariableNode>(variableNode->getName());
                    expandedRight = std::make_unique<BinaryExpNode>(std::move(leftVar), binaryOp, std::move(rightExpr));

                    return std::make_unique<AssignmentNode>(variableNode->getName(),
                                                            std::move(expandedRight),
                                                            ValueType::VOID, "");
                }
            }
        }

        return expr;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseTernary()
    {
        auto expr = parseLogicalOr();

        if (tokenStream.match(TokenType::QUESTION))
        {
            auto trueExpr = parseExpression();
            tokenStream.expect(TokenType::COLON);
            auto falseExpr = parseExpression();
            return std::make_unique<TernaryExpNode>(std::move(expr), std::move(trueExpr), std::move(falseExpr));
        }

        return expr;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseLogicalOr()
    {
        auto left = parseLogicalAnd();

        while (tokenStream.current().type == TokenType::OR)
        {
            TokenType op = tokenStream.current().type;
            tokenStream.advance();
            auto right = parseLogicalAnd();
            left = std::make_unique<BinaryExpNode>(std::move(left), op, std::move(right));
        }

        return left;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseLogicalAnd()
    {
        auto left = parseEquality();

        while (tokenStream.current().type == TokenType::AND)
        {
            TokenType op = tokenStream.current().type;
            tokenStream.advance();
            auto right = parseEquality();
            left = std::make_unique<BinaryExpNode>(std::move(left), op, std::move(right));
        }

        return left;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseEquality()
    {
        auto left = parseComparison();

        while (tokenStream.current().type == TokenType::EQUALS || tokenStream.current().type ==
            TokenType::NOT_EQUALS)
        {
            TokenType op = tokenStream.current().type;
            tokenStream.advance();
            auto right = parseComparison();
            left = std::make_unique<BinaryExpNode>(std::move(left), op, std::move(right));
        }

        return left;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseComparison()
    {
        auto left = parseAdditive();

        while (tokenStream.current().type == TokenType::LESS || tokenStream.current().type ==
            TokenType::LESS_EQUALS ||
            tokenStream.current().type == TokenType::GREATER || tokenStream.current().type ==
            TokenType::GREATER_EQUALS)
        {
            TokenType op = tokenStream.current().type;
            tokenStream.advance();
            auto right = parseAdditive();
            left = std::make_unique<BinaryExpNode>(std::move(left), op, std::move(right));
        }

        return left;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseAdditive()
    {
        auto left = parseMultiplicative();

        while (tokenStream.current().type == TokenType::PLUS || tokenStream.current().type == TokenType::MINUS)
        {
            TokenType op = tokenStream.current().type;
            tokenStream.advance();
            auto right = parseMultiplicative();
            left = std::make_unique<BinaryExpNode>(std::move(left), op, std::move(right));
        }

        return left;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseMultiplicative()
    {
        auto left = parseUnary();

        while (tokenStream.current().type == TokenType::MULTIPLY || tokenStream.current().type ==
            TokenType::DIVIDE ||
            tokenStream.current().type == TokenType::MODULO)
        {
            TokenType op = tokenStream.current().type;
            tokenStream.advance();
            auto right = parseUnary();
            left = std::make_unique<BinaryExpNode>(std::move(left), op, std::move(right));
        }

        return left;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseUnary()
    {
        // Handle prefix unary operators like !, -, +, ++, --
        if (tokenStream.current().type == TokenType::NOT ||
            tokenStream.current().type == TokenType::MINUS ||
            tokenStream.current().type == TokenType::PLUS ||
            tokenStream.current().type == TokenType::INCREMENT ||
            tokenStream.current().type == TokenType::DECREMENT)
        {
            TokenType op = tokenStream.current().type;
            tokenStream.advance();
            auto operand = parseUnary();
            return std::make_unique<UnaryExpNode>(op, std::move(operand),
                                                  ast::nodes::expressions::UnaryPosition::PREFIX);
        }

        return parsePostfix();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parsePostfix()
    {
        auto expr = parsePrimary();

        while (true)
        {
            if (tokenStream.current().type == TokenType::INCREMENT ||
                tokenStream.current().type == TokenType::DECREMENT)
            {
                // Postfix increment/decrement
                TokenType op = tokenStream.current().type;
                tokenStream.advance();
                // Create a postfix operation using UnaryExpNode with POSTFIX position
                expr = std::make_unique<UnaryExpNode>(op, std::move(expr),
                                                      ast::nodes::expressions::UnaryPosition::POSTFIX);
            }
            else if (tokenStream.current().type == TokenType::LPAREN)
            {
                // Function call
                std::string funcName;
                bool canCallFunction = false;

                if (auto varNode = dynamic_cast<VariableNode*>(expr.get()))
                {
                    funcName = varNode->getName();
                    canCallFunction = true;
                }

                if (canCallFunction)
                {
                    tokenStream.advance(); // Skip '('
                    auto arguments = parseArguments();
                    tokenStream.expect(TokenType::RPAREN);
                    // No need to call release() - unique_ptr will handle cleanup automatically
                    expr = std::make_unique<FunctionCallNode>(funcName, std::move(arguments));
                }
            }
            else if (tokenStream.current().type == TokenType::DOT)
            {
                // Member access
                expr = parseMemberAccess(std::move(expr));
            }
            else if (tokenStream.current().type == TokenType::SCOPE)
            {
                tokenStream.advance();
                if (tokenStream.current().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected identifier after '::'", tokenStream.current().location);
                }

                if (auto varNode = dynamic_cast<VariableNode*>(expr.get()))
                {
                    std::vector<std::string> parts = {varNode->getName()};

                    // Add the first identifier after ::
                    parts.push_back(tokenStream.current().stringValue);
                    tokenStream.advance();

                    // Continue parsing if there are more :: tokens
                    while (tokenStream.current().type == TokenType::SCOPE)
                    {
                        tokenStream.advance();
                        if (tokenStream.current().type != TokenType::IDENTIFIER)
                        {
                            throw ParseException("Expected identifier after '::'", tokenStream.current().location);
                        }
                        parts.push_back(tokenStream.current().stringValue);
                        tokenStream.advance();
                    }

                    // Check if this is a function call (e.g., MathUtils::max(10, 5))
                    if (tokenStream.current().type == TokenType::LPAREN)
                    {
                        // Join the parts to create the full function name
                        std::string fullName = parts[0];
                        for (size_t i = 1; i < parts.size(); i++)
                        {
                            fullName += "::" + parts[i];
                        }

                        // Parse as function call using the same method as normal function calls
                        tokenStream.advance(); // consume '('
                        auto arguments = parseArguments();
                        tokenStream.expect(TokenType::RPAREN);

                        expr = std::make_unique<nodes::functions::FunctionCallNode>(fullName, std::move(arguments));
                    }
                    else
                    {
                        // This is a qualified variable access (e.g., TestClass::myField)
                        // Join the parts to create the full variable name
                        std::string fullName = parts[0];
                        for (size_t i = 1; i < parts.size(); i++)
                        {
                            fullName += "::" + parts[i];
                        }

                        // Create a VariableNode with the qualified name
                        expr = std::make_unique<nodes::expressions::VariableNode>(fullName);
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
        switch (tokenStream.current().type)
        {
        case TokenType::INT_NUMBER:
            {
                int value = tokenStream.current().intValue;
                tokenStream.advance();
                return std::make_unique<IntegerNode>(value);
            }
        case TokenType::FLOAT_NUMBER:
            {
                float value = tokenStream.current().floatValue;
                tokenStream.advance();
                return std::make_unique<FloatNode>(value);
            }
        case TokenType::STRING_LITERAL:
            {
                std::string value = tokenStream.current().stringValue;
                tokenStream.advance();
                return std::make_unique<StringNode>(value);
            }
        case TokenType::TRUE:
            {
                tokenStream.advance();
                return std::make_unique<BoolNode>(true);
            }
        case TokenType::FALSE:
            {
                tokenStream.advance();
                return std::make_unique<BoolNode>(false);
            }
        case TokenType::NULL_LITERAL:
            {
                tokenStream.advance();
                return std::make_unique<NullNode>();
            }
        case TokenType::IDENTIFIER:
            {
                std::string name = tokenStream.current().stringValue;
                tokenStream.advance();
                return std::make_unique<VariableNode>(name);
            }
        case TokenType::LPAREN:
            {
                tokenStream.advance();
                auto expr = parseExpression();
                tokenStream.expect(TokenType::RPAREN);
                return expr;
            }
        case TokenType::NEW:
            {
                // Delegate to ClassParser for proper separation of concerns
                return context.parseNewExpression();
            }
        default:
            throw ParseException("Unexpected token in primary expression", tokenStream.current().location);
        }
    }

    std::vector<std::unique_ptr<ASTNode>> ExpressionParser::parseArguments()
    {
        std::vector<std::unique_ptr<ASTNode>> arguments;

        if (tokenStream.current().type != TokenType::RPAREN)
        {
            arguments.push_back(parseExpression());

            while (tokenStream.match(TokenType::COMMA))
            {
                arguments.push_back(parseExpression());
            }
        }

        return arguments;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseMemberAccess(std::unique_ptr<ASTNode> object)
    {
        tokenStream.expect(TokenType::DOT);

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected member name after '.'", tokenStream.current().location);
        }

        std::string memberName = tokenStream.current().stringValue;
        tokenStream.advance();

        // Check if it's a method call
        if (tokenStream.current().type == TokenType::LPAREN)
        {
            tokenStream.advance();
            auto arguments = parseArguments();
            tokenStream.expect(TokenType::RPAREN);
            return std::make_unique<MethodCallNode>(std::move(object), memberName, std::move(arguments));
        }
        else
        {
            return std::make_unique<MemberAccessNode>(std::move(object), memberName);
        }
    }
}
