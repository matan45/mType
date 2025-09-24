#include "ExpressionParser.hpp"
#include <iostream>
#include "TypeParser.hpp"
#include "ParserUtils.hpp"
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
#include "../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../ast/nodes/expressions/ArrayLiteralNode.hpp"
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
            SourceLocation assignmentLocation = tokenStream.current().location;  // Capture location before advancing
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
                        std::move(rightExpr),
                        assignmentLocation);
                }
                else
                {
                    // Compound member assignment - not fully implemented yet
                    throw ParseException("Compound assignment to member not yet supported",
                                         assignmentLocation);
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
                        std::move(rightExpr),
                        assignmentLocation);
                }
                else
                {
                    // Compound index assignment - not fully implemented yet
                    throw ParseException("Compound assignment to index not yet supported",
                                         assignmentLocation);
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
                                                            ValueType::VOID, "",
                                                            false, false,
                                                            assignmentLocation);
                }
                else
                {
                    // Compound assignment - we need to expand this to: var = var op right
                    std::unique_ptr<ASTNode> expandedRight;
                    TokenType binaryOp;

                    binaryOp = ParserUtils::compoundToBinaryOperator(opType);

                    // Create: var op right
                    auto leftVar = std::make_unique<VariableNode>(variableNode->getName(), assignmentLocation);
                    expandedRight = std::make_unique<BinaryExpNode>(std::move(leftVar), binaryOp, std::move(rightExpr), assignmentLocation);

                    return std::make_unique<AssignmentNode>(variableNode->getName(),
                                                            std::move(expandedRight),
                                                            ValueType::VOID, "",
                                                            false, false,
                                                            assignmentLocation);
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

                    // Parse generic type arguments if present (e.g., Box::method<String, Int>)
                    std::vector<std::string> genericTypeArguments;
                    if (tokenStream.current().type == TokenType::LESS)
                    {
                        tokenStream.advance(); // consume '<'
                        genericTypeArguments = parseGenericTypeArguments();
                        tokenStream.expect(TokenType::GREATER); // consume '>'
                    }

                    // Check if this is a function call (e.g., MathUtils::max(10, 5))
                    if (tokenStream.current().type == TokenType::LPAREN)
                    {
                        // Check if we have generic type arguments - if so, treat as static generic method call
                        if (!genericTypeArguments.empty() && parts.size() >= 2)
                        {
                            std::string className = parts[0];
                            std::string methodName = parts[1];

                            // Parse arguments
                            tokenStream.advance(); // consume '('
                            auto arguments = parseArguments();
                            tokenStream.expect(TokenType::RPAREN);

                            // Create MethodCallNode for static generic call
                            auto classNodeLocation = varNode->getLocation(); // Use location from the original variable node
                            auto classNode = std::make_unique<nodes::expressions::VariableNode>(className, classNodeLocation);
                            expr = std::make_unique<nodes::classes::MethodCallNode>(
                                std::move(classNode), methodName, std::move(arguments),
                                true, genericTypeArguments, classNodeLocation); // isStatic = true
                        }
                        else
                        {
                            // Regular function call - use existing logic
                            std::string fullName = ParserUtils::buildQualifiedName(parts);
                            tokenStream.advance(); // consume '('
                            auto arguments = parseArguments();
                            tokenStream.expect(TokenType::RPAREN);

                            expr = std::make_unique<nodes::functions::FunctionCallNode>(fullName, std::move(arguments));
                        }
                    }
                    else
                    {
                        // This is a qualified variable access (e.g., TestClass::myField)
                        // Efficiently build the full variable name
                        std::string fullName = ParserUtils::buildQualifiedName(parts);

                        // Create a VariableNode with the qualified name
                        auto qualifiedVarLocation = varNode->getLocation(); // Use location from the original variable node
                        expr = std::make_unique<nodes::expressions::VariableNode>(fullName, qualifiedVarLocation);
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
                auto intLocation = tokenStream.current().location;
                tokenStream.advance();
                return std::make_unique<IntegerNode>(value, intLocation);
            }
        case TokenType::FLOAT_NUMBER:
            {
                float value = tokenStream.current().floatValue;
                auto floatLocation = tokenStream.current().location;
                tokenStream.advance();
                return std::make_unique<FloatNode>(value, floatLocation);
            }
        case TokenType::STRING_LITERAL:
            {
                std::string value = tokenStream.current().stringValue.getString();
                auto stringLocation = tokenStream.current().location;
                tokenStream.advance();
                return std::make_unique<StringNode>(value, stringLocation);
            }
        case TokenType::TRUE:
            {
                auto trueLocation = tokenStream.current().location;
                tokenStream.advance();
                return std::make_unique<BoolNode>(true, trueLocation);
            }
        case TokenType::FALSE:
            {
                auto falseLocation = tokenStream.current().location;
                tokenStream.advance();
                return std::make_unique<BoolNode>(false, falseLocation);
            }
        case TokenType::NULL_LITERAL:
            {
                auto nullLocation = tokenStream.current().location;
                tokenStream.advance();
                return std::make_unique<NullNode>(nullLocation);
            }
        case TokenType::IDENTIFIER:
            {
                std::string name = tokenStream.current().stringValue.getString();
                auto identifierLocation = tokenStream.current().location;
                tokenStream.advance();
                return std::make_unique<VariableNode>(name, identifierLocation);
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
                // Parse array literal: [1, 2, 3]
                return parseArrayLiteral();
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

        std::string memberName = tokenStream.current().stringValue.getString();
        SourceLocation location = tokenStream.current().location;
        tokenStream.advance();

        // Check if it's a method call
        if (tokenStream.current().type == TokenType::LPAREN)
        {
            tokenStream.advance();
            auto arguments = parseArguments();
            tokenStream.expect(TokenType::RPAREN);
            return std::make_unique<MethodCallNode>(std::move(object), memberName, std::move(arguments), false, std::vector<std::string>(), location);
        }
        else
        {
            return std::make_unique<MemberAccessNode>(std::move(object), memberName, false, location);
        }
    }





    std::unique_ptr<ASTNode> ExpressionParser::parseIndexAccess(std::unique_ptr<ASTNode> collection)
    {
        // Parse index access: collection[index]
        SourceLocation location = tokenStream.current().location;  // Capture location of the '[' token
        tokenStream.expect(TokenType::LBRACKET);

        auto index = parseExpression();
        tokenStream.expect(TokenType::RBRACKET);

        return std::make_unique<IndexAccessNode>(std::move(collection), std::move(index), location);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseArrayLiteral()
    {
        // Parse array literal: [element1, element2, element3, ...]
        SourceLocation location = tokenStream.current().location;  // Capture location of the '[' token
        tokenStream.expect(TokenType::LBRACKET);

        std::vector<std::unique_ptr<ASTNode>> elements;

        // Handle empty array: []
        if (tokenStream.check(TokenType::RBRACKET))
        {
            tokenStream.advance(); // consume ']'
            return std::make_unique<ArrayLiteralNode>(std::move(elements), location);
        }

        // Parse first element
        elements.push_back(parseExpression());

        // Parse additional elements separated by commas
        while (tokenStream.check(TokenType::COMMA))
        {
            tokenStream.advance(); // consume ','
            elements.push_back(parseExpression());
        }

        tokenStream.expect(TokenType::RBRACKET);

        return std::make_unique<ArrayLiteralNode>(std::move(elements), location);
    }

    std::vector<std::string> ExpressionParser::parseGenericTypeArguments()
    {
        std::vector<std::string> typeArgs;

        // Parse first type argument
        typeArgs.push_back(parseGenericTypeArgument());

        // Parse additional arguments separated by commas
        while (tokenStream.check(TokenType::COMMA))
        {
            tokenStream.advance(); // consume ','
            typeArgs.push_back(parseGenericTypeArgument());
        }

        return typeArgs;
    }

    std::string ExpressionParser::parseGenericTypeArgument()
    {
        std::string typeArg;

        if (tokenStream.current().type == TokenType::IDENTIFIER)
        {
            typeArg = tokenStream.current().stringValue.getString();
            tokenStream.advance();

            // Handle nested generics like Array<String>
            if (tokenStream.check(TokenType::LESS))
            {
                typeArg += "<";
                tokenStream.advance(); // consume '<'

                // Recursively parse nested type arguments
                auto nestedArgs = parseGenericTypeArguments();
                for (size_t i = 0; i < nestedArgs.size(); ++i)
                {
                    if (i > 0) typeArg += ",";
                    typeArg += nestedArgs[i];
                }

                tokenStream.expect(TokenType::GREATER); // consume '>'
                typeArg += ">";
            }

            return typeArg;
        }
        else
        {
            throw ParseException("Expected type argument", tokenStream.current().location);
        }
    }
}
