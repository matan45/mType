#include "ExpressionParser.hpp"
#include "TypeParser.hpp"
#include "ParserUtils.hpp"
#include "LambdaParser.hpp"
#include "../services/ImportManager.hpp"
#include "expression/BinaryOperatorParser.hpp"
#include "expression/UnaryOperatorParser.hpp"
#include "expression/PostfixOperatorParser.hpp"
#include "expression/LiteralParser.hpp"
#include "expression/ArgumentParser.hpp"
#include "../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../ast/nodes/expressions/VariableNode.hpp"
#include "../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../ast/nodes/classes/MemberAccessNode.hpp"
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

    void ExpressionParser::initializeHelperParsers()
    {
        binaryOpParser = std::make_unique<BinaryOperatorParser>(tokenStream, context, errorHandler);
        unaryOpParser = std::make_unique<UnaryOperatorParser>(tokenStream, context, errorHandler);
        postfixOpParser = std::make_unique<PostfixOperatorParser>(tokenStream, context, errorHandler);
        literalParser = std::make_unique<LiteralParser>(tokenStream, context, errorHandler);
        argumentParser = std::make_unique<ArgumentParser>(tokenStream, context, errorHandler);

        // Set ExpressionParser reference in parsers to break circular dependencies
        binaryOpParser->setExpressionParser(*this);
        unaryOpParser->setExpressionParser(*this);
        postfixOpParser->setExpressionParser(*this);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseExpression()
    {
        return parseAssignment();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseAssignment()
    {
        auto expr = parseLambda();

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
            SourceLocation assignmentLocation = tokenStream.current().location; // Capture location before advancing
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
                    expandedRight = std::make_unique<BinaryExpNode>(std::move(leftVar), binaryOp, std::move(rightExpr),
                                                                    assignmentLocation);

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

    std::unique_ptr<ASTNode> ExpressionParser::parseLambda()
    {
        // Check if this looks like a lambda expression
        if (isLambdaStart())
        {
            LambdaParser lambdaParser(tokenStream, context);
            return lambdaParser.parseLambda();
        }

        // Not a lambda, delegate to next precedence level
        return parseTernary();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseTernary()
    {
        return binaryOpParser->parseTernary();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseLogicalOr()
    {
        return binaryOpParser->parseLogicalOr();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseLogicalAnd()
    {
        return binaryOpParser->parseLogicalAnd();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseEquality()
    {
        return binaryOpParser->parseEquality();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseComparison()
    {
        return binaryOpParser->parseComparison();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseAdditive()
    {
        return binaryOpParser->parseAdditive();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseMultiplicative()
    {
        return binaryOpParser->parseMultiplicative();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseUnary()
    {
        return unaryOpParser->parseUnary();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parsePostfix()
    {
        return postfixOpParser->parsePostfix();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parsePrimary()
    {
        return literalParser->parsePrimary();
    }

    std::vector<std::unique_ptr<ASTNode>> ExpressionParser::parseArguments()
    {
        return argumentParser->parseArguments();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseMemberAccess(std::unique_ptr<ASTNode> object)
    {
        return postfixOpParser->parseMemberAccess(std::move(object));
    }


    std::unique_ptr<ASTNode> ExpressionParser::parseIndexAccess(std::unique_ptr<ASTNode> collection)
    {
        return postfixOpParser->parseIndexAccess(std::move(collection));
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseArrayLiteral()
    {
        return literalParser->parseArrayLiteral();
    }

    std::vector<std::string> ExpressionParser::parseGenericTypeArguments()
    {
        return argumentParser->parseGenericTypeArguments();
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

    bool ExpressionParser::isLambdaStart() const
    {
        // Pattern 1: identifier -> (single parameter without parentheses)
        if (tokenStream.current().type == TokenType::IDENTIFIER)
        {
            if (!tokenStream.isAtEnd() && tokenStream.peek().type == TokenType::ARROW)
            {
                return true;
            }
        }

        // Pattern 2: ( ... ) -> (parenthesized parameters or empty)
        if (tokenStream.current().type == TokenType::LPAREN)
        {
            // Check for empty parameter list: () ->
            Token next = tokenStream.peek();
            if (next.type == TokenType::RPAREN)
            {
                return true; // Likely () -> pattern
            }

            // For multi-parameter lambdas, use more sophisticated heuristic
            if (next.type == TokenType::IDENTIFIER)
            {
                // This could be either:
                // 1. (param1, param2) -> ... (lambda parameters)
                // 2. (variable + expression) ... (parenthesized expression)

                // Heuristic: lambda parameters are more likely if we're at assignment context
                // For now, be conservative - assume it's a parenthesized expression unless
                // we have stronger evidence it's lambda parameters

                // Look for lambda-like patterns: identifier followed by comma or colon
                return isLikelyLambdaParameterList();
            }

            // Otherwise it's likely a parenthesized expression, not lambda parameters
            return false;
        }

        return false;
    }

    bool ExpressionParser::isLikelyLambdaParameterList() const
    {
        // Look for very specific lambda patterns only:
        // Pattern 1: (a, b) -> ... (comma-separated identifiers)
        // Pattern 2: (a : int, b : int) -> ... (typed parameters)
        // Pattern 3: (a) -> ... (single parameter)

        Token token1 = tokenStream.peek(); // First token after (
        if (token1.type != TokenType::IDENTIFIER)
        {
            return false;
        }

        // Check if we can look ahead safely
        if (tokenStream.isAtEnd())
        {
            return false;
        }

        // Look for the pattern (identifier, ...)
        // If we see (identifier, identifier) or (identifier :) it's likely lambda
        // If we see (identifier +) or (identifier *) it's likely expression

        // Use the safe peekAhead for just the next couple tokens
        try
        {
            Token token2 = tokenStream.peekAhead(2); // Second token after identifier

            // Pattern: (identifier, ...) suggests lambda parameters
            if (token2.type == TokenType::COMMA)
            {
                return true;
            }

            // Pattern: (identifier : ...) suggests typed parameter
            if (token2.type == TokenType::COLON)
            {
                return true;
            }

            // Pattern: (identifier ) -> suggests single parameter lambda
            if (token2.type == TokenType::RPAREN)
            {
                Token token3 = tokenStream.peekAhead(3);
                if (token3.type == TokenType::ARROW)
                {
                    return true;
                }
            }

            // Otherwise assume it's a parenthesized expression
            return false;
        }
        catch (...)
        {
            return false;
        }
    }
}
