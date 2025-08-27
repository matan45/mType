#include "ExpressionParser.hpp"
#include "Parser.hpp"
#include "../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../ast/nodes/expressions/IntegerNode.hpp"
#include "../ast/nodes/expressions/FloatNode.hpp"
#include "../ast/nodes/expressions/StringNode.hpp"
#include "../ast/nodes/expressions/BoolNode.hpp"
#include "../ast/nodes/expressions/NullNode.hpp"
#include "../ast/nodes/expressions/VariableNode.hpp"
#include "../ast/nodes/functions/FunctionCallNode.hpp"

namespace parser
{
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace token;

    std::unique_ptr<ASTNode> ExpressionParser::parseExpression()
    {
        return parseTernary();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseTernary()
    {
        auto expr = parseLogicalOr();
        
        if (parser.matchToken(TokenType::QUESTION)) {
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
        
        while (parser.getCurrentToken().type == TokenType::OR) {
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
        
        while (parser.getCurrentToken().type == TokenType::AND) {
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
        
        while (parser.getCurrentToken().type == TokenType::EQUALS || parser.getCurrentToken().type == TokenType::NOT_EQUALS) {
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
        
        while (parser.getCurrentToken().type == TokenType::LESS || parser.getCurrentToken().type == TokenType::LESS_EQUALS ||
               parser.getCurrentToken().type == TokenType::GREATER || parser.getCurrentToken().type == TokenType::GREATER_EQUALS) {
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
        
        while (parser.getCurrentToken().type == TokenType::PLUS || parser.getCurrentToken().type == TokenType::MINUS) {
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
        
        while (parser.getCurrentToken().type == TokenType::MULTIPLY || parser.getCurrentToken().type == TokenType::DIVIDE || 
               parser.getCurrentToken().type == TokenType::MODULO) {
            TokenType op = parser.getCurrentToken().type;
            parser.advanceToken();
            auto right = parseUnary();
            left = std::make_unique<BinaryExpNode>(std::move(left), op, std::move(right));
        }
        
        return left;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseUnary()
    {
        // Handle unary operators like !, -, +
        if (parser.getCurrentToken().type == TokenType::NOT || 
            parser.getCurrentToken().type == TokenType::MINUS || 
            parser.getCurrentToken().type == TokenType::PLUS) {
            TokenType op = parser.getCurrentToken().type;
            parser.advanceToken();
            auto operand = parseUnary();
            return std::make_unique<BinaryExpNode>(nullptr, op, std::move(operand)); // Unary represented as binary with null left
        }
        
        return parsePostfix();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parsePostfix()
    {
        auto expr = parsePrimary();
        
        while (true) {
            if (parser.getCurrentToken().type == TokenType::LPAREN) {
                // Function call
                if (auto varNode = dynamic_cast<VariableNode*>(expr.get())) {
                    std::string funcName = varNode->getName();
                    expr.release(); // Release ownership since we're replacing it
                    
                    parser.advanceToken(); // Skip '('
                    std::vector<std::unique_ptr<ASTNode>> arguments;
                    
                    if (parser.getCurrentToken().type != TokenType::RPAREN) {
                        arguments.push_back(parseExpression());
                        
                        while (parser.matchToken(TokenType::COMMA)) {
                            arguments.push_back(parseExpression());
                        }
                    }
                    
                    parser.expectToken(TokenType::RPAREN);
                    expr = std::make_unique<FunctionCallNode>(funcName, std::move(arguments));
                }
            } else if (parser.getCurrentToken().type == TokenType::DOT) {
                // Member access
                expr = parseMemberAccess(std::move(expr));
            } else {
                break;
            }
        }
        
        return expr;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parsePrimary()
    {
        switch (parser.getCurrentToken().type) {
            case TokenType::INT_NUMBER: {
                int value = parser.getCurrentToken().intValue;
                parser.advanceToken();
                return std::make_unique<IntegerNode>(value);
            }
            case TokenType::FLOAT_NUMBER: {
                float value = parser.getCurrentToken().floatValue;
                parser.advanceToken();
                return std::make_unique<FloatNode>(value);
            }
            case TokenType::STRING_LITERAL: {
                std::string value = parser.getCurrentToken().stringValue;
                parser.advanceToken();
                return std::make_unique<StringNode>(value);
            }
            case TokenType::TRUE: {
                parser.advanceToken();
                return std::make_unique<BoolNode>(true);
            }
            case TokenType::FALSE: {
                parser.advanceToken();
                return std::make_unique<BoolNode>(false);
            }
            case TokenType::NULL_LITERAL: {
                parser.advanceToken();
                return std::make_unique<NullNode>();
            }
            case TokenType::IDENTIFIER: {
                std::string name = parser.getCurrentToken().stringValue;
                parser.advanceToken();
                return std::make_unique<VariableNode>(name);
            }
            case TokenType::LPAREN: {
                parser.advanceToken();
                auto expr = parseExpression();
                parser.expectToken(TokenType::RPAREN);
                return expr;
            }
            default:
                throw std::runtime_error("Unexpected token in primary expression");
        }
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseArguments()
    {
        // This method would parse argument lists for function calls
        // Implementation would be similar to parameter parsing
        return nullptr;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseMemberAccess(std::unique_ptr<ASTNode> object)
    {
        parser.expectToken(TokenType::DOT);
        
        if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected member name after '.'");
        }
        
        std::string memberName = parser.getCurrentToken().stringValue;
        parser.advanceToken();
        
        // For now, return null - would need MemberAccessNode implementation
        // return std::make_unique<MemberAccessNode>(std::move(object), memberName);
        return nullptr;
    }
}
