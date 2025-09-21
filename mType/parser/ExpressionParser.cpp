#include "ExpressionParser.hpp"
#include "TypeParser.hpp"
#include "ParserUtils.hpp"
#include "../services/ImportManager.hpp"
#include <iostream>
#include "../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../ast/nodes/expressions/IntegerNode.hpp"
#include "../ast/nodes/expressions/FloatNode.hpp"
#include "../ast/nodes/expressions/StringNode.hpp"
#include "../ast/nodes/expressions/BoolNode.hpp"
#include "../ast/nodes/expressions/NullNode.hpp"
#include "../ast/nodes/expressions/VariableNode.hpp"
#include "../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../ast/nodes/expressions/MapLiteralNode.hpp"
#include "../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../ast/nodes/functions/FunctionCallNode.hpp"
#include "../ast/nodes/classes/MemberAccessNode.hpp"
#include "../ast/nodes/classes/MethodCallNode.hpp"
#include "../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../ast/nodes/statements/IndexAssignmentNode.hpp"
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
            // For assignment expressions, the left side should be a variable, member access, or index access
            auto variableNode = dynamic_cast<VariableNode*>(expr.get());
            auto memberAccessNode = dynamic_cast<MemberAccessNode*>(expr.get());
            auto indexAccessNode = dynamic_cast<IndexAccessNode*>(expr.get());
            if (!variableNode && !memberAccessNode && !indexAccessNode)
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
                        memberAccessNode->transferObjectOwnership(),
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
            else if (indexAccessNode)
            {
                // Index assignment (e.g., array[0] = "value")
                if (opType == TokenType::ASSIGN)
                {
                    return std::make_unique<IndexAssignmentNode>(
                        indexAccessNode->transferCollectionOwnership(),
                        indexAccessNode->transferIndexOwnership(),
                        std::move(rightExpr));
                }
                else
                {
                    // Compound index assignment - not fully implemented yet
                    throw ParseException("Compound assignment to index not yet supported",
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

                    binaryOp = ParserUtils::compoundToBinaryOperator(opType);

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
        return ParserUtils::parseBinaryOperators(
            tokenStream,
            [this]() { return parseLogicalAnd(); },
            {TokenType::OR}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseLogicalAnd()
    {
        return ParserUtils::parseBinaryOperators(
            tokenStream,
            [this]() { return parseEquality(); },
            {TokenType::AND}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseEquality()
    {
        return ParserUtils::parseBinaryOperators(
            tokenStream,
            [this]() { return parseComparison(); },
            {TokenType::EQUALS, TokenType::NOT_EQUALS}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseComparison()
    {
        return ParserUtils::parseBinaryOperators(
            tokenStream,
            [this]() { return parseAdditive(); },
            {TokenType::LESS, TokenType::LESS_EQUALS, TokenType::GREATER, TokenType::GREATER_EQUALS}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseAdditive()
    {
        return ParserUtils::parseBinaryOperators(
            tokenStream,
            [this]() { return parseMultiplicative(); },
            {TokenType::PLUS, TokenType::MINUS}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseMultiplicative()
    {
        return ParserUtils::parseBinaryOperators(
            tokenStream,
            [this]() { return parseUnary(); },
            {TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::MODULO}
        );
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
            else if (tokenStream.current().type == TokenType::LBRACKET)
            {
                // Index access: array[index]
                expr = parseIndexAccess(std::move(expr));
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
                    // Use centralized qualified identifier chain parsing
                    auto parts = ParserUtils::parseQualifiedIdentifierChain(tokenStream, varNode->getName());

                    // Check if this is a function call (e.g., MathUtils::max(10, 5))
                    if (tokenStream.current().type == TokenType::LPAREN)
                    {
                        // Efficiently build the full function name
                        std::string fullName = ParserUtils::buildQualifiedName(parts);

                        // Parse as function call using the same method as normal function calls
                        tokenStream.advance(); // consume '('
                        auto arguments = parseArguments();
                        tokenStream.expect(TokenType::RPAREN);

                        expr = std::make_unique<nodes::functions::FunctionCallNode>(fullName, std::move(arguments));
                    }
                    else
                    {
                        // This is a qualified variable access (e.g., TestClass::myField)
                        // Efficiently build the full variable name
                        std::string fullName = ParserUtils::buildQualifiedName(parts);

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
        case TokenType::LBRACKET:
            {
                // Array literal: [1, 2, 3]
                return parseArrayLiteral();
            }
        case TokenType::LBRACE:
            {
                // Map literal: {"key": value}
                return parseMapLiteral();
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

    std::unique_ptr<ASTNode> ExpressionParser::parseArrayLiteral()
    {
        // Parse array literal: [element1, element2, ...]
        tokenStream.expect(TokenType::LBRACKET);
        
        // Handle empty array
        if (tokenStream.current().type == TokenType::RBRACKET)
        {
            tokenStream.advance();
            // For empty arrays, use OBJECT as default - type will be refined by context
            auto arrayNode = std::make_unique<ArrayLiteralNode>(ValueType::OBJECT);
            return std::move(arrayNode);
        }
        
        // Try to infer element type from the first element
        ValueType inferredType = inferArrayElementType();
        auto arrayNode = std::make_unique<ArrayLiteralNode>(inferredType);
        
        // Parse elements
        do {
            auto element = parseExpression();
            arrayNode->addElement(std::move(element));
            
            if (tokenStream.current().type == TokenType::COMMA)
            {
                tokenStream.advance();
            }
            else if (tokenStream.current().type == TokenType::RBRACKET)
            {
                break;
            }
            else
            {
                throw ParseException("Expected ',' or ']' in array literal", tokenStream.current().location);
            }
        } while (true);
        
        tokenStream.expect(TokenType::RBRACKET);
        return std::move(arrayNode);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseMapLiteral()
    {
        // Parse map literal: {"key": value, "key2": value2}
        tokenStream.expect(TokenType::LBRACE);
        
        // Handle empty map
        if (tokenStream.current().type == TokenType::RBRACE)
        {
            tokenStream.advance();
            // For empty maps, use OBJECT as default - type will be refined by context
            auto mapNode = std::make_unique<MapLiteralNode>(ValueType::OBJECT, ValueType::OBJECT);
            return std::move(mapNode);
        }
        
        // Try to infer key and value types from the first pair
        ValueType keyType = inferArrayElementType(); // reuse the logic
        
        // Parse the first key to move to the value
        auto firstKey = parseExpression();
        tokenStream.expect(TokenType::COLON);
        
        ValueType valueType = inferArrayElementType();
        auto mapNode = std::make_unique<MapLiteralNode>(keyType, valueType);
        
        // Add the first key-value pair
        auto firstValue = parseExpression();
        mapNode->addKeyValuePair(std::move(firstKey), std::move(firstValue));
        
        // Continue parsing additional key-value pairs if there are any
        while (tokenStream.current().type == TokenType::COMMA)
        {
            tokenStream.advance(); // consume ','
            
            auto key = parseExpression();
            tokenStream.expect(TokenType::COLON);
            auto value = parseExpression();
            
            mapNode->addKeyValuePair(std::move(key), std::move(value));
        }
        
        tokenStream.expect(TokenType::RBRACE);
        return std::move(mapNode);
    }

    ValueType ExpressionParser::inferArrayElementType()
    {
        // Look ahead at the first token to infer the element type
        TokenType currentType = tokenStream.current().type;
        
        switch (currentType)
        {
        case TokenType::INT_NUMBER:
            return ValueType::INT;
        case TokenType::FLOAT_NUMBER:
            return ValueType::FLOAT;
        case TokenType::STRING_LITERAL:
            return ValueType::STRING;
        case TokenType::TRUE:
        case TokenType::FALSE:
            return ValueType::BOOL;
        case TokenType::NULL_LITERAL:
            return ValueType::OBJECT; // null can be any object type
        case TokenType::NEW:
            return ValueType::OBJECT; // object construction
        case TokenType::IDENTIFIER:
            // Could be a variable reference - assume object for now
            return ValueType::OBJECT;
        case TokenType::LBRACKET:
            return ValueType::ARRAY; // nested array
        case TokenType::LBRACE:
            return ValueType::MAP; // nested map
        default:
            // Default to OBJECT for complex expressions
            return ValueType::OBJECT;
        }
    }

    ValueType ExpressionParser::inferElementTypeFromToken()
    {
        // Helper method to infer type from current token without advancing
        return inferArrayElementType();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseIndexAccess(std::unique_ptr<ASTNode> collection)
    {
        // Parse index access: collection[index]
        tokenStream.expect(TokenType::LBRACKET);
        
        auto index = parseExpression();
        tokenStream.expect(TokenType::RBRACKET);
        
        return std::make_unique<IndexAccessNode>(std::move(collection), std::move(index));
    }
}
