#include "ExpressionParser.hpp"
#include <cstddef>
#include "LambdaParser.hpp"
#include "TypeParser.hpp"
#include "utilities/ParserUtils.hpp"
#include "utilities/QualifiedNameParser.hpp"
#include "../ast/nodes/classes/MemberAccessNode.hpp"
#include "../ast/nodes/classes/MethodCallNode.hpp"
#include "../ast/nodes/classes/SuperConstructorCallNode.hpp"
#include "../ast/nodes/classes/SuperMemberAccessNode.hpp"
#include "../ast/nodes/classes/SuperMemberAssignmentNode.hpp"
#include "../ast/nodes/classes/SuperMethodCallNode.hpp"
#include "../ast/nodes/classes/ThisConstructorCallNode.hpp"
#include "../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../ast/nodes/expressions/AwaitExpression.hpp"
#include "../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../ast/nodes/expressions/BoolNode.hpp"
#include "../ast/nodes/expressions/CastExpression.hpp"
#include "../ast/nodes/expressions/FloatNode.hpp"
#include "../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../ast/nodes/expressions/InstanceOfExpression.hpp"
#include "../ast/nodes/expressions/IntegerNode.hpp"
#include "../ast/nodes/expressions/LambdaNode.hpp"
#include "../ast/nodes/expressions/NullNode.hpp"
#include "../ast/nodes/expressions/StringNode.hpp"
#include "../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../ast/nodes/expressions/VariableNode.hpp"
#include "../ast/nodes/functions/FunctionCallNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/statements/IndexAssignmentNode.hpp"
#include "../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../errors/MissingSemicolonException.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::classes;
    using namespace token;
    using namespace value;
    using namespace errors;

    // ============================================================
    // Construction and BaseParser-equivalent helpers
    // ============================================================

    ExpressionParser::ExpressionParser(TokenStream& stream, ParseContext& ctx)
        : tokenStream(stream), context(ctx)
    {
    }

    void ExpressionParser::expectToken(TokenType type)
    {
        if (!tokenStream.check(type))
        {
            if (type == TokenType::SEMICOLON)
            {
                throw MissingSemicolonException(tokenStream.current().location);
            }
            throw ParseException("Expected token but found different token", tokenStream.current().location);
        }
        tokenStream.advance();
    }

    bool ExpressionParser::tryConsumeToken(TokenType type)
    {
        if (tokenStream.check(type))
        {
            tokenStream.advance();
            return true;
        }
        return false;
    }

    const Token& ExpressionParser::currentToken() const
    {
        return tokenStream.current();
    }

    bool ExpressionParser::isAtEnd() const
    {
        return tokenStream.isAtEnd();
    }

    // ============================================================
    // Top-level parseExpression + parseAssignment
    // ============================================================

    std::unique_ptr<ASTNode> ExpressionParser::parseExpression()
    {
        return parseAssignment();
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseAssignment()
    {
        auto expr = parseLambda();

        if (tokenStream.current().type == TokenType::ASSIGN ||
            tokenStream.current().type == TokenType::PLUS_ASSIGN ||
            tokenStream.current().type == TokenType::MINUS_ASSIGN ||
            tokenStream.current().type == TokenType::MULTIPLY_ASSIGN ||
            tokenStream.current().type == TokenType::DIVIDE_ASSIGN ||
            tokenStream.current().type == TokenType::MODULO_ASSIGN)
        {
            auto variableNode = dynamic_cast<VariableNode*>(expr.get());
            auto memberAccessNode = dynamic_cast<MemberAccessNode*>(expr.get());
            auto superMemberAccessNode = dynamic_cast<SuperMemberAccessNode*>(expr.get());
            auto indexAccessNode = dynamic_cast<IndexAccessNode*>(expr.get());

            if (!variableNode && !memberAccessNode && !superMemberAccessNode && !indexAccessNode)
            {
                throw ParseException("Invalid assignment target", tokenStream.current().location);
            }

            TokenType opType = tokenStream.current().type;
            SourceLocation assignmentLocation = tokenStream.current().location;
            tokenStream.advance();
            auto rightExpr = parseAssignment();

            if (superMemberAccessNode)
            {
                return handleSuperMemberAssignment(superMemberAccessNode, opType, std::move(rightExpr), assignmentLocation);
            }
            else if (memberAccessNode)
            {
                return handleMemberAssignment(memberAccessNode, opType, std::move(rightExpr), assignmentLocation);
            }
            else if (indexAccessNode)
            {
                return handleIndexAssignment(indexAccessNode, opType, std::move(rightExpr), assignmentLocation);
            }
            else if (variableNode)
            {
                return handleVariableAssignment(variableNode, opType, std::move(rightExpr), assignmentLocation);
            }
        }

        return expr;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseLambda()
    {
        bool isAsync = false;
        if (tokenStream.current().type == TokenType::ASYNC)
        {
            Token nextToken = tokenStream.peek();
            if (nextToken.type == TokenType::IDENTIFIER || nextToken.type == TokenType::LPAREN)
            {
                tokenStream.advance();
                isAsync = true;
            }
        }

        if (isLambdaStart())
        {
            LambdaParser lambdaParser(tokenStream, context);
            auto lambdaNode = lambdaParser.parseLambda();

            if (isAsync)
            {
                static_cast<LambdaNode*>(lambdaNode.get())->setIsAsync(true);
            }

            return lambdaNode;
        }

        return parseTernary();
    }

    // ============================================================
    // Binary operators (absorbed from BinaryOperatorParser)
    // ============================================================

    std::unique_ptr<ASTNode> ExpressionParser::parseTernary()
    {
        ParserContextState::RecursionDepthGuard depthGuard(context.getContextState());

        auto expr = parseLogicalOr();

        if (tokenStream.check(TokenType::QUESTION))
        {
            SourceLocation questionLocation = tokenStream.current().location;
            tokenStream.advance();
            auto trueExpr = parseTernary();
            expectToken(TokenType::COLON);
            auto falseExpr = parseTernary();
            return std::make_unique<TernaryExpNode>(std::move(expr), std::move(trueExpr), std::move(falseExpr),
                                                    questionLocation);
        }

        return expr;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseLogicalOr()
    {
        return parseBinaryLevel(
            [this]() { return parseLogicalAnd(); },
            {TokenType::OR}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseLogicalAnd()
    {
        return parseBinaryLevel(
            [this]() { return parseBitwiseOr(); },
            {TokenType::AND}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseBitwiseOr()
    {
        return parseBinaryLevel(
            [this]() { return parseBitwiseXor(); },
            {TokenType::BITWISE_OR}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseBitwiseXor()
    {
        return parseBinaryLevel(
            [this]() { return parseBitwiseAnd(); },
            {TokenType::BITWISE_XOR}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseBitwiseAnd()
    {
        return parseBinaryLevel(
            [this]() { return parseEquality(); },
            {TokenType::BITWISE_AND}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseEquality()
    {
        return parseBinaryLevel(
            [this]() { return parseComparison(); },
            {TokenType::EQUALS, TokenType::NOT_EQUALS}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseComparison()
    {
        return parseBinaryLevel(
            [this]() { return parseIsClassOf(); },
            {TokenType::LESS, TokenType::LESS_EQUALS, TokenType::GREATER, TokenType::GREATER_EQUALS}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseIsClassOf()
    {
        auto left = parseShift();

        while (tokenStream.check(TokenType::ISCLASSOF))
        {
            left = parseInstanceOfExpression(std::move(left));
        }

        return left;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseShift()
    {
        return parseBinaryLevel(
            [this]() { return parseAdditive(); },
            {TokenType::LEFT_SHIFT, TokenType::RIGHT_SHIFT}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseAdditive()
    {
        return parseBinaryLevel(
            [this]() { return parseMultiplicative(); },
            {TokenType::PLUS, TokenType::MINUS}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseMultiplicative()
    {
        return parseBinaryLevel(
            [this]() { return parseUnary(); },
            {TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::MODULO}
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseBinaryLevel(
        std::function<std::unique_ptr<ASTNode>()> parseNext,
        const std::vector<TokenType>& operators)
    {
        ParserContextState::RecursionDepthGuard depthGuard(context.getContextState());

        try
        {
            return ParserUtils::parseBinaryOperators(tokenStream, parseNext, operators);
        }
        catch (const ParseException&)
        {
            throw;
        }
        catch (const std::exception& e)
        {
            throw ParseException("Binary operator parsing failed: " + std::string(e.what()),
                                 tokenStream.current().location);
        }
    }

    // ============================================================
    // Unary (absorbed from UnaryOperatorParser)
    // ============================================================

    std::unique_ptr<ASTNode> ExpressionParser::parseUnary()
    {
        ParserContextState::RecursionDepthGuard depthGuard(context.getContextState());

        if (canParseCast())
        {
            return parseCastExpression();
        }

        if (tokenStream.current().type == TokenType::AWAIT)
        {
            SourceLocation awaitLocation = tokenStream.current().location;
            tokenStream.advance();

            if (context.isInsideConstructorBody())
            {
                throw ParseException("'await' cannot be used inside constructors (constructors cannot be async)",
                                     awaitLocation);
            }

            bool isInNonAsyncFunction = context.isInsideFunctionBody() && !context.isInsideAsyncFunction();

            if (isInNonAsyncFunction)
            {
                throw ParseException("'await' can only be used inside async functions or at the top level",
                                     awaitLocation);
            }

            auto expression = parseUnary();
            return std::make_unique<AwaitExpression>(std::move(expression), awaitLocation);
        }

        switch (tokenStream.current().type)
        {
        case TokenType::NOT:
        case TokenType::MINUS:
        case TokenType::PLUS:
        case TokenType::INCREMENT:
        case TokenType::DECREMENT:
        case TokenType::BITWISE_NOT:
            {
                TokenType op = tokenStream.current().type;
                SourceLocation opLocation = tokenStream.current().location;
                tokenStream.advance();
                auto operand = parseUnary();
                return std::make_unique<UnaryExpNode>(op, std::move(operand), UnaryPosition::PREFIX, opLocation);
            }
        default:
            break;
        }

        return parsePostfix();
    }

    // ============================================================
    // Cast / instanceof (absorbed from CastParser)
    // ============================================================

    bool ExpressionParser::canParseCast() const
    {
        if (!tokenStream.check(TokenType::LPAREN))
        {
            return false;
        }
        try
        {
            return isCastExpression();
        }
        catch (...)
        {
            return false;
        }
    }

    bool ExpressionParser::isCastExpression() const
    {
        Token nextToken = tokenStream.peek();
        TokenType type = nextToken.type;

        if (type == TokenType::INT || type == TokenType::FLOAT ||
            type == TokenType::BOOL || type == TokenType::STRING_TYPE ||
            type == TokenType::VOID)
        {
            return true;
        }

        if (type == TokenType::IDENTIFIER)
        {
            Token afterId = tokenStream.peekAhead(2);

            if (afterId.type == TokenType::RPAREN)
            {
                return true;
            }

            if (afterId.type == TokenType::LBRACKET)
            {
                std::string idValue = std::string(nextToken.stringValue);
                if (!idValue.empty() && std::isupper(idValue[0]))
                {
                    return true;
                }
            }

            if (afterId.type == TokenType::LESS)
            {
                std::string idValue = std::string(nextToken.stringValue);
                if (!idValue.empty() && std::isupper(idValue[0]))
                {
                    return true;
                }
            }

            if (afterId.type == TokenType::QUESTION)
            {
                Token afterQuestion = tokenStream.peekAhead(3);
                if (afterQuestion.type == TokenType::RPAREN)
                {
                    std::string idValue = std::string(nextToken.stringValue);
                    if (!idValue.empty() && std::isupper(static_cast<unsigned char>(idValue[0])))
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseCastExpression()
    {
        SourceLocation startLoc = tokenStream.current().location;

        expectToken(TokenType::LPAREN);

        SourceLocation typeStart = tokenStream.current().location;
        auto targetType = TypeParser::parseGenericType(tokenStream);

        if (targetType->isNullable())
        {
            throw ParseException(
                "Nullable cast syntax (T?) is not supported. "
                "Cast to T and assign to a T? variable, or use null directly.",
                typeStart);
        }

        expectToken(TokenType::RPAREN);

        auto expr = parseUnary();

        return std::make_unique<CastExpression>(
            std::move(expr),
            std::move(targetType),
            startLoc
        );
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseInstanceOfExpression(std::unique_ptr<ASTNode> left)
    {
        SourceLocation loc = tokenStream.current().location;

        if (!tokenStream.check(TokenType::ISCLASSOF))
        {
            throw ParseException("Expected 'isClassOf'", tokenStream.current().location);
        }
        tokenStream.advance();

        auto targetType = TypeParser::parseGenericType(tokenStream);

        return std::make_unique<InstanceOfExpression>(
            std::move(left),
            std::move(targetType),
            loc
        );
    }

    // ============================================================
    // Postfix (absorbed from PostfixOperatorParser)
    // ============================================================

    std::unique_ptr<ASTNode> ExpressionParser::parsePostfix()
    {
        ParserContextState::RecursionDepthGuard depthGuard(context.getContextState());

        auto expr = parsePrimary();
        return parsePostfixOperations(std::move(expr));
    }

    std::unique_ptr<ASTNode> ExpressionParser::parsePostfixOperations(std::unique_ptr<ASTNode> expr)
    {
        while (true)
        {
            if (tokenStream.check(TokenType::INCREMENT) || tokenStream.check(TokenType::DECREMENT))
            {
                TokenType op = tokenStream.current().type;
                SourceLocation opLocation = tokenStream.current().location;
                tokenStream.advance();
                expr = std::make_unique<UnaryExpNode>(op, std::move(expr), UnaryPosition::POSTFIX, opLocation);
            }
            else if (tokenStream.check(TokenType::LESS) && dynamic_cast<VariableNode*>(expr.get()) &&
                isGenericFunctionCall())
            {
                expr = parseFunctionCall(std::move(expr));
            }
            else if (tokenStream.check(TokenType::LPAREN))
            {
                expr = parseFunctionCall(std::move(expr));
            }
            else if (tokenStream.check(TokenType::DOT))
            {
                expr = parseMemberAccess(std::move(expr));
            }
            else if (tokenStream.check(TokenType::QUESTION_DOT))
            {
                expr = parseMemberAccess(std::move(expr), /*isSafe*/ true);
            }
            else if (tokenStream.check(TokenType::LBRACKET))
            {
                expr = parseIndexAccess(std::move(expr));
            }
            else if (tokenStream.check(TokenType::SCOPE))
            {
                expr = parseScopeResolution(std::move(expr));
            }
            else
            {
                break;
            }
        }

        return expr;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseFunctionCall(std::unique_ptr<ASTNode> expr)
    {
        std::string funcName;
        bool canCallFunction = false;
        SourceLocation callLocation;

        if (auto varNode = dynamic_cast<VariableNode*>(expr.get()))
        {
            funcName = varNode->getName();
            canCallFunction = true;
            callLocation = varNode->getLocation();
        }

        if (canCallFunction)
        {
            if (funcName == "this" && context.isInsideConstructorBody())
            {
                std::vector<std::unique_ptr<ASTNode>> arguments = parseArgumentsWithParentheses();
                return std::make_unique<ThisConstructorCallNode>(std::move(arguments), callLocation);
            }

            std::vector<std::string> genericTypeArguments;
            if (tokenStream.check(TokenType::LESS))
            {
                tokenStream.advance();
                genericTypeArguments = parseGenericTypeArguments();
                tokenStream.expectGreaterForGeneric();
            }

            std::vector<std::unique_ptr<ASTNode>> arguments = parseArgumentsWithParentheses();
            return std::make_unique<FunctionCallNode>(funcName, std::move(arguments), genericTypeArguments,
                                                      tokenStream.current().location);
        }

        throw ParseException("Invalid function call target", tokenStream.current().location);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseMemberAccess(std::unique_ptr<ASTNode> object, bool isSafe)
    {
        expectToken(isSafe ? TokenType::QUESTION_DOT : TokenType::DOT);

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            throw ParseException(isSafe ? "Expected member name after '?.'" : "Expected member name after '.'",
                                 tokenStream.current().location);
        }

        std::string memberName = std::string(tokenStream.current().stringValue);
        SourceLocation location = tokenStream.current().location;
        tokenStream.advance();

        std::vector<std::string> genericTypeArguments;
        if (tokenStream.check(TokenType::LESS) && isGenericFunctionCall())
        {
            tokenStream.advance();
            genericTypeArguments = parseGenericTypeArguments();
            tokenStream.expectGreaterForGeneric();
        }

        if (tokenStream.check(TokenType::LPAREN))
        {
            tokenStream.advance();
            std::vector<std::unique_ptr<ASTNode>> arguments;

            if (!tokenStream.check(TokenType::RPAREN))
            {
                arguments.push_back(context.parseExpression());

                while (tokenStream.check(TokenType::COMMA))
                {
                    tokenStream.advance();
                    arguments.push_back(context.parseExpression());
                }
            }

            expectToken(TokenType::RPAREN);
            return std::make_unique<MethodCallNode>(std::move(object), memberName, std::move(arguments),
                                                    false, genericTypeArguments, location, isSafe);
        }

        return std::make_unique<MemberAccessNode>(std::move(object), memberName, false, location, isSafe);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseIndexAccess(std::unique_ptr<ASTNode> collection)
    {
        SourceLocation location = tokenStream.current().location;
        expectToken(TokenType::LBRACKET);

        auto index = context.parseExpression();
        expectToken(TokenType::RBRACKET);

        return std::make_unique<IndexAccessNode>(std::move(collection), std::move(index), location);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseScopeResolution(std::unique_ptr<ASTNode> expr)
    {
        expectToken(TokenType::SCOPE);

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            throw ParseException("Expected identifier after '::'", tokenStream.current().location);
        }

        if (auto varNode = dynamic_cast<VariableNode*>(expr.get()))
        {
            auto parts = QualifiedNameParser::parseQualifiedIdentifierChain(tokenStream, varNode->getName());

            std::vector<std::string> genericTypeArguments;
            if (tokenStream.check(TokenType::LESS))
            {
                tokenStream.advance();
                genericTypeArguments = parseGenericTypeArguments();
                tokenStream.expectGreaterForGeneric();
            }

            if (tokenStream.check(TokenType::LPAREN))
            {
                if (!genericTypeArguments.empty() && parts.size() >= 2)
                {
                    return parseGenericMethodCall(parts[0], parts[1], genericTypeArguments);
                }
                else
                {
                    std::string fullName = QualifiedNameParser::buildQualifiedName(parts);
                    std::vector<std::unique_ptr<ASTNode>> arguments = parseArgumentsWithParentheses();
                    return std::make_unique<FunctionCallNode>(fullName, std::move(arguments),
                                                              tokenStream.current().location);
                }
            }
            else
            {
                std::string fullName = QualifiedNameParser::buildQualifiedName(parts);
                auto qualifiedVarLocation = varNode->getLocation();
                return std::make_unique<VariableNode>(fullName, qualifiedVarLocation);
            }
        }

        throw ParseException("Invalid scope resolution target", tokenStream.current().location);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseGenericMethodCall(
        const std::string& className,
        const std::string& methodName,
        const std::vector<std::string>& typeArgs)
    {
        expectToken(TokenType::LPAREN);
        std::vector<std::unique_ptr<ASTNode>> arguments;

        if (!tokenStream.check(TokenType::RPAREN))
        {
            arguments.push_back(context.parseExpression());

            while (tokenStream.check(TokenType::COMMA))
            {
                tokenStream.advance();
                arguments.push_back(context.parseExpression());
            }
        }

        expectToken(TokenType::RPAREN);

        auto classNodeLocation = tokenStream.current().location;
        auto classNode = std::make_unique<VariableNode>(className, classNodeLocation);
        return std::make_unique<MethodCallNode>(std::move(classNode), methodName, std::move(arguments),
                                                true, typeArgs, classNodeLocation);
    }

    bool ExpressionParser::isPostfixOperator(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::INCREMENT:
        case TokenType::DECREMENT:
        case TokenType::LPAREN:
        case TokenType::DOT:
        case TokenType::LBRACKET:
        case TokenType::SCOPE:
            return true;
        default:
            return false;
        }
    }

    bool ExpressionParser::isGenericFunctionCall()
    {
        if (!tokenStream.check(TokenType::LESS))
        {
            return false;
        }

        try
        {
            size_t offset = 1;
            int depth = 1;

            while (depth > 0)
            {
                Token nextToken = tokenStream.peekAhead(offset);

                if (nextToken.type == TokenType::END)
                {
                    return false;
                }

                if (nextToken.type == TokenType::LESS)
                {
                    depth++;
                }
                else if (nextToken.type == TokenType::GREATER)
                {
                    depth--;
                    if (depth == 0)
                    {
                        Token afterGreater = tokenStream.peekAhead(offset + 1);
                        return afterGreater.type == TokenType::LPAREN;
                    }
                }
                else if (nextToken.type == TokenType::RIGHT_SHIFT)
                {
                    depth -= 2;
                    if (depth == 0)
                    {
                        Token afterRightShift = tokenStream.peekAhead(offset + 1);
                        return afterRightShift.type == TokenType::LPAREN;
                    }
                    else if (depth < 0)
                    {
                        Token afterRightShift = tokenStream.peekAhead(offset + 1);
                        return afterRightShift.type == TokenType::LPAREN;
                    }
                }
                else if (nextToken.type != TokenType::IDENTIFIER &&
                    nextToken.type != TokenType::COMMA)
                {
                    return false;
                }

                offset++;

                if (offset > 100)
                {
                    return false;
                }
            }

            return false;
        }
        catch (...)
        {
            return false;
        }
    }

    // ============================================================
    // Primary / literals (absorbed from LiteralParser)
    // ============================================================

    std::unique_ptr<ASTNode> ExpressionParser::parsePrimary()
    {
        switch (tokenStream.current().type)
        {
        case TokenType::INT_NUMBER:
            return parseIntegerLiteral();
        case TokenType::FLOAT_NUMBER:
            return parseFloatLiteral();
        case TokenType::STRING_LITERAL:
            return parseStringLiteral();
        case TokenType::TRUE:
        case TokenType::FALSE:
            return parseBooleanLiteral();
        case TokenType::NULL_LITERAL:
            return parseNullLiteral();
        case TokenType::IDENTIFIER:
            return parseIdentifier();
        case TokenType::LPAREN:
            return parseParenthesizedExpression();
        case TokenType::NEW:
            return context.parseNewExpression();
        case TokenType::LBRACKET:
            return parseArrayLiteral();
        case TokenType::SUPER:
            return parseSuperExpression();
        case TokenType::INTERP_STRING_BEGIN:
            return parseInterpolatedStringExpression();
        default:
            throw ParseException("Unexpected token in primary expression", tokenStream.current().location);
        }
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseArrayLiteral()
    {
        SourceLocation location = tokenStream.current().location;
        expectToken(TokenType::LBRACKET);

        std::vector<std::unique_ptr<ASTNode>> elements;

        if (tokenStream.check(TokenType::RBRACKET))
        {
            tokenStream.advance();
            return std::make_unique<ArrayLiteralNode>(std::move(elements), location);
        }

        elements.push_back(context.parseExpression());

        while (tryConsumeToken(TokenType::COMMA))
        {
            elements.push_back(context.parseExpression());
        }

        expectToken(TokenType::RBRACKET);
        return std::make_unique<ArrayLiteralNode>(std::move(elements), location);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseIntegerLiteral()
    {
        int value = static_cast<int>(tokenStream.current().intValue);
        auto intLocation = tokenStream.current().location;
        tokenStream.advance();
        return std::make_unique<IntegerNode>(value, intLocation);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseFloatLiteral()
    {
        double value = tokenStream.current().floatValue;
        auto floatLocation = tokenStream.current().location;
        tokenStream.advance();
        return std::make_unique<FloatNode>(value, floatLocation);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseStringLiteral()
    {
        std::string value = std::string(tokenStream.current().stringValue);
        auto stringLocation = tokenStream.current().location;
        tokenStream.advance();
        return std::make_unique<StringNode>(value, stringLocation);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseBooleanLiteral()
    {
        bool value = (tokenStream.current().type == TokenType::TRUE);
        auto boolLocation = tokenStream.current().location;
        tokenStream.advance();
        return std::make_unique<BoolNode>(value, boolLocation);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseNullLiteral()
    {
        auto nullLocation = tokenStream.current().location;
        tokenStream.advance();
        return std::make_unique<NullNode>(nullLocation);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseIdentifier()
    {
        std::string name = std::string(tokenStream.current().stringValue);
        auto identifierLocation = tokenStream.current().location;
        tokenStream.advance();
        return std::make_unique<VariableNode>(name, identifierLocation);
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseParenthesizedExpression()
    {
        expectToken(TokenType::LPAREN);
        auto expr = context.parseExpression();
        expectToken(TokenType::RPAREN);
        return expr;
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseSuperExpression()
    {
        SourceLocation superLocation = tokenStream.current().location;
        expectToken(TokenType::SUPER);

        if (tokenStream.check(TokenType::LPAREN))
        {
            if (context.isInsideConstructorBody())
            {
                throw ParseException(
                    "super() constructor calls are not allowed inside constructor bodies. "
                    "Use the constructor initializer list syntax instead: constructor(...) : super(...) { }",
                    superLocation);
            }

            std::vector<std::unique_ptr<ASTNode>> arguments = parseArgumentsWithParentheses();

            return std::make_unique<SuperConstructorCallNode>(
                std::move(arguments), superLocation);
        }
        else if (tokenStream.check(TokenType::DOT))
        {
            tokenStream.advance();

            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected member name after 'super.'",
                                     tokenStream.current().location);
            }

            std::string memberName = std::string(tokenStream.current().stringValue);
            tokenStream.advance();

            if (tokenStream.check(TokenType::LPAREN))
            {
                std::vector<std::unique_ptr<ASTNode>> arguments = parseArgumentsWithParentheses();

                return std::make_unique<SuperMethodCallNode>(
                    memberName, std::move(arguments), superLocation);
            }
            else
            {
                return std::make_unique<SuperMemberAccessNode>(
                    memberName, superLocation);
            }
        }
        else
        {
            throw ParseException("Expected '(' or '.' after 'super'",
                                 tokenStream.current().location);
        }
    }

    std::unique_ptr<ASTNode> ExpressionParser::parseInterpolatedStringExpression()
    {
        SourceLocation location = tokenStream.current().location;
        bool hasConcatenation = false;

        auto makePlus = [&](std::unique_ptr<ASTNode> left, std::unique_ptr<ASTNode> right)
        {
            hasConcatenation = true;
            return std::make_unique<BinaryExpNode>(
                std::move(left), TokenType::PLUS, std::move(right), location);
        };

        std::string beginText = std::string(tokenStream.current().stringValue);
        tokenStream.advance();

        std::unique_ptr<ASTNode> result;

        if (!beginText.empty())
        {
            result = std::make_unique<StringNode>(beginText, location);
        }

        auto expr = context.parseExpression();

        if (result)
        {
            result = makePlus(std::move(result), std::move(expr));
        }
        else
        {
            result = std::move(expr);
        }

        while (tokenStream.check(TokenType::INTERP_STRING_MIDDLE))
        {
            std::string middleText = std::string(tokenStream.current().stringValue);
            tokenStream.advance();

            if (!middleText.empty())
            {
                result = makePlus(std::move(result),
                                  std::make_unique<StringNode>(middleText, location));
            }

            expr = context.parseExpression();
            result = makePlus(std::move(result), std::move(expr));
        }

        if (!tokenStream.check(TokenType::INTERP_STRING_END))
        {
            throw ParseException("Expected end of interpolated string",
                                 tokenStream.current().location);
        }

        std::string endText = std::string(tokenStream.current().stringValue);
        tokenStream.advance();

        if (!endText.empty())
        {
            result = makePlus(std::move(result),
                              std::make_unique<StringNode>(endText, location));
        }

        if (!hasConcatenation)
        {
            result = makePlus(std::make_unique<StringNode>("", location), std::move(result));
        }

        return result;
    }

    // ============================================================
    // Argument lists (absorbed from ArgumentParser)
    // ============================================================

    std::vector<std::unique_ptr<ASTNode>> ExpressionParser::parseArguments()
    {
        std::vector<std::unique_ptr<ASTNode>> arguments;

        if (!tokenStream.check(TokenType::RPAREN))
        {
            arguments.push_back(context.parseExpression());

            while (tryConsumeToken(TokenType::COMMA))
            {
                arguments.push_back(context.parseExpression());
            }
        }

        return arguments;
    }

    std::vector<std::unique_ptr<ASTNode>> ExpressionParser::parseArgumentsWithParentheses()
    {
        expectToken(TokenType::LPAREN);
        auto arguments = parseArguments();
        expectToken(TokenType::RPAREN);
        return arguments;
    }

    std::vector<std::string> ExpressionParser::parseGenericTypeArguments()
    {
        std::vector<std::string> typeArgs;

        typeArgs.push_back(parseGenericTypeArgument());

        while (tryConsumeToken(TokenType::COMMA))
        {
            typeArgs.push_back(parseGenericTypeArgument());
        }

        return typeArgs;
    }

    std::string ExpressionParser::parseGenericTypeArgument()
    {
        std::string typeArg;

        if (tokenStream.check(TokenType::IDENTIFIER))
        {
            typeArg = std::string(tokenStream.current().stringValue);
            tokenStream.advance();

            if (tokenStream.check(TokenType::LESS))
            {
                typeArg += "<";
                tokenStream.advance();

                auto nestedArgs = parseGenericTypeArguments();
                for (size_t i = 0; i < nestedArgs.size(); ++i)
                {
                    if (i > 0) typeArg += ",";
                    typeArg += nestedArgs[i];
                }

                tokenStream.expectGreaterForGeneric();
                typeArg += ">";
            }

            return typeArg;
        }

        throw ParseException("Expected type argument", tokenStream.current().location);
    }

    // ============================================================
    // Lambda detection
    // ============================================================

    bool ExpressionParser::isLambdaStart() const
    {
        if (tokenStream.current().type == TokenType::IDENTIFIER)
        {
            if (!tokenStream.isAtEnd() && tokenStream.peek().type == TokenType::ARROW)
            {
                return true;
            }
        }

        if (tokenStream.current().type == TokenType::LPAREN)
        {
            Token next = tokenStream.peek();
            if (next.type == TokenType::RPAREN)
            {
                return true;
            }

            if (next.type == TokenType::IDENTIFIER)
            {
                return isLikelyLambdaParameterList();
            }

            return false;
        }

        return false;
    }

    bool ExpressionParser::isLikelyLambdaParameterList() const
    {
        Token token1 = tokenStream.peek();
        if (token1.type != TokenType::IDENTIFIER)
        {
            return false;
        }

        if (tokenStream.isAtEnd())
        {
            return false;
        }

        try
        {
            Token token2 = tokenStream.peekAhead(2);

            if (token2.type == TokenType::COMMA)
            {
                return true;
            }

            if (token2.type == TokenType::COLON)
            {
                return true;
            }

            if (token2.type == TokenType::RPAREN)
            {
                Token token3 = tokenStream.peekAhead(3);
                if (token3.type == TokenType::ARROW)
                {
                    return true;
                }
            }

            return false;
        }
        catch (...)
        {
            return false;
        }
    }

    // ============================================================
    // Assignment helpers (originals)
    // ============================================================

    std::unique_ptr<ASTNode> ExpressionParser::handleMemberAssignment(
        MemberAccessNode* memberAccessNode,
        TokenType opType,
        std::unique_ptr<ASTNode> rightExpr,
        const SourceLocation& location)
    {
        if (opType == TokenType::ASSIGN)
        {
            return std::make_unique<MemberAssignmentNode>(
                memberAccessNode->getObjectShared(),
                memberAccessNode->getMemberName(),
                std::move(rightExpr),
                location);
        }
        else
        {
            TokenType binaryOp = ParserUtils::compoundToBinaryOperator(opType);

            auto readNode = std::make_unique<MemberAccessNode>(
                memberAccessNode->getObjectShared(),
                memberAccessNode->getMemberName(),
                memberAccessNode->getIsStaticAccess(),
                location);

            auto expandedRight = std::make_unique<BinaryExpNode>(
                std::move(readNode), binaryOp, std::move(rightExpr), location);

            return std::make_unique<MemberAssignmentNode>(
                memberAccessNode->getObjectShared(),
                memberAccessNode->getMemberName(),
                std::move(expandedRight),
                location);
        }
    }

    std::unique_ptr<ASTNode> ExpressionParser::handleSuperMemberAssignment(
        SuperMemberAccessNode* superMemberAccessNode,
        TokenType opType,
        std::unique_ptr<ASTNode> rightExpr,
        const SourceLocation& location)
    {
        if (opType == TokenType::ASSIGN)
        {
            return std::make_unique<SuperMemberAssignmentNode>(
                superMemberAccessNode->getMemberName(),
                std::move(rightExpr),
                location);
        }
        else
        {
            TokenType binaryOp = ParserUtils::compoundToBinaryOperator(opType);

            auto readNode = std::make_unique<SuperMemberAccessNode>(
                superMemberAccessNode->getMemberName(), location);

            auto expandedRight = std::make_unique<BinaryExpNode>(
                std::move(readNode), binaryOp, std::move(rightExpr), location);

            return std::make_unique<SuperMemberAssignmentNode>(
                superMemberAccessNode->getMemberName(),
                std::move(expandedRight),
                location);
        }
    }

    std::unique_ptr<ASTNode> ExpressionParser::handleIndexAssignment(
        IndexAccessNode* indexAccessNode,
        TokenType opType,
        std::unique_ptr<ASTNode> rightExpr,
        const SourceLocation& location)
    {
        if (opType == TokenType::ASSIGN)
        {
            return std::make_unique<IndexAssignmentNode>(
                indexAccessNode->transferCollectionOwnership(),
                indexAccessNode->transferIndexOwnership(),
                std::move(rightExpr),
                location);
        }
        else
        {
            TokenType binaryOp = ParserUtils::compoundToBinaryOperator(opType);

            auto collectionClone = indexAccessNode->getCollection()->clone();
            auto indexClone = indexAccessNode->getIndex()->clone();

            auto readNode = std::make_unique<IndexAccessNode>(
                std::move(collectionClone), std::move(indexClone), location);

            auto expandedRight = std::make_unique<BinaryExpNode>(
                std::move(readNode), binaryOp, std::move(rightExpr), location);

            return std::make_unique<IndexAssignmentNode>(
                indexAccessNode->transferCollectionOwnership(),
                indexAccessNode->transferIndexOwnership(),
                std::move(expandedRight),
                location);
        }
    }

    std::unique_ptr<ASTNode> ExpressionParser::handleVariableAssignment(
        VariableNode* variableNode,
        TokenType opType,
        std::unique_ptr<ASTNode> rightExpr,
        const SourceLocation& location)
    {
        if (opType == TokenType::ASSIGN)
        {
            return std::make_unique<AssignmentNode>(variableNode->getName(),
                                                    std::move(rightExpr),
                                                    ValueType::VOID, "",
                                                    false, false,
                                                    location);
        }
        else
        {
            TokenType binaryOp = ParserUtils::compoundToBinaryOperator(opType);

            auto leftVar = std::make_unique<VariableNode>(variableNode->getName(), location);
            auto expandedRight = std::make_unique<BinaryExpNode>(std::move(leftVar), binaryOp, std::move(rightExpr),
                                                                 location);

            return std::make_unique<AssignmentNode>(variableNode->getName(),
                                                    std::move(expandedRight),
                                                    ValueType::VOID, "",
                                                    false, false,
                                                    location);
        }
    }
}
