#include "PostfixOperatorParser.hpp"
#include "../ExpressionParser.hpp"
#include "../ParserUtils.hpp"
#include "../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../ast/nodes/expressions/VariableNode.hpp"
#include "../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../exceptions/DomainExceptions.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::expression
{
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::classes;
    using namespace token;
    using namespace errors;

    std::unique_ptr<ASTNode> PostfixOperatorParser::parse()
    {
        // This should not be called directly - use parsePostfix
        reportError("PostfixOperatorParser::parse() called directly", getParserName());
        throw errors::ParseException("Invalid use of PostfixOperatorParser");
    }

    std::unique_ptr<ASTNode> PostfixOperatorParser::parsePostfix()
    {
        if (!expressionParser)
        {
            reportError("ExpressionParser not set in PostfixOperatorParser", getParserName());
            throw errors::ParseException("ExpressionParser not initialized in PostfixOperatorParser");
        }
        auto expr = expressionParser->parsePrimary(); // Delegate to ExpressionParser for primary parsing
        return parsePostfixOperations(std::move(expr));
    }

    bool PostfixOperatorParser::canParse(const TokenStream& stream) const
    {
        return isPostfixOperator(stream.current().type);
    }

    std::unique_ptr<ASTNode> PostfixOperatorParser::parsePostfixOperations(std::unique_ptr<ASTNode> expr)
    {
        while (true)
        {
            if (tokenStream.check(TokenType::INCREMENT) || tokenStream.check(TokenType::DECREMENT))
            {
                // Postfix increment/decrement
                TokenType op = tokenStream.current().type;
                tokenStream.advance();
                expr = std::make_unique<UnaryExpNode>(op, std::move(expr), UnaryPosition::POSTFIX);
            }
            else if (tokenStream.check(TokenType::LPAREN))
            {
                // Function call
                expr = parseFunctionCall(std::move(expr));
            }
            else if (tokenStream.check(TokenType::DOT))
            {
                // Member access
                expr = parseMemberAccess(std::move(expr));
            }
            else if (tokenStream.check(TokenType::LBRACKET))
            {
                // Index access: array[index]
                expr = parseIndexAccess(std::move(expr));
            }
            else if (tokenStream.check(TokenType::SCOPE))
            {
                // Scope resolution
                expr = parseScopeResolution(std::move(expr));
            }
            else
            {
                break;
            }
        }

        return expr;
    }

    std::unique_ptr<ASTNode> PostfixOperatorParser::parseFunctionCall(std::unique_ptr<ASTNode> expr)
    {
        std::string funcName;
        bool canCallFunction = false;

        if (auto varNode = dynamic_cast<VariableNode*>(expr.get()))
        {
            funcName = varNode->getName();
            canCallFunction = true;
        }

        if (canCallFunction)
        {
            expectToken(TokenType::LPAREN, getParserName());
            std::vector<std::unique_ptr<ASTNode>> arguments;

            // Parse arguments: arg1, arg2, arg3, ...
            if (!tokenStream.check(TokenType::RPAREN))
            {
                arguments.push_back(context.parseExpression());

                while (tokenStream.check(TokenType::COMMA))
                {
                    tokenStream.advance(); // consume ','
                    arguments.push_back(context.parseExpression());
                }
            }

            expectToken(TokenType::RPAREN, getParserName());
            return std::make_unique<FunctionCallNode>(funcName, std::move(arguments), getCurrentLocation());
        }
        else
        {
            reportError("Invalid function call target", getParserName());
            throw errors::ParseException("Invalid function call target");
        }
    }

    std::unique_ptr<ASTNode> PostfixOperatorParser::parseMemberAccess(std::unique_ptr<ASTNode> object)
    {
        expectToken(TokenType::DOT, getParserName());

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            reportError("Expected member name after '.'", getParserName());
            throw errors::ParseException("Expected member name after '.'");
        }

        std::string memberName = tokenStream.current().stringValue.getString();
        SourceLocation location = getCurrentLocation();
        tokenStream.advance();

        // Check if it's a method call
        if (tokenStream.check(TokenType::LPAREN))
        {
            tokenStream.advance();
            std::vector<std::unique_ptr<ASTNode>> arguments;

            // Parse arguments: arg1, arg2, arg3, ...
            if (!tokenStream.check(TokenType::RPAREN))
            {
                arguments.push_back(context.parseExpression());

                while (tokenStream.check(TokenType::COMMA))
                {
                    tokenStream.advance(); // consume ','
                    arguments.push_back(context.parseExpression());
                }
            }

            expectToken(TokenType::RPAREN, getParserName());
            return std::make_unique<MethodCallNode>(std::move(object), memberName, std::move(arguments),
                                                   false, std::vector<std::string>(), location);
        }
        else
        {
            return std::make_unique<MemberAccessNode>(std::move(object), memberName, false, location);
        }
    }

    std::unique_ptr<ASTNode> PostfixOperatorParser::parseIndexAccess(std::unique_ptr<ASTNode> collection)
    {
        SourceLocation location = getCurrentLocation();
        expectToken(TokenType::LBRACKET, getParserName());

        auto index = context.parseExpression();
        expectToken(TokenType::RBRACKET, getParserName());

        return std::make_unique<IndexAccessNode>(std::move(collection), std::move(index), location);
    }

    std::unique_ptr<ASTNode> PostfixOperatorParser::parseScopeResolution(std::unique_ptr<ASTNode> expr)
    {
        expectToken(TokenType::SCOPE, getParserName());

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            reportError("Expected identifier after '::'", getParserName());
            throw errors::ParseException("Expected identifier after '::'");
        }

        if (auto varNode = dynamic_cast<VariableNode*>(expr.get()))
        {
            // Use centralized qualified identifier chain parsing
            auto parts = ParserUtils::parseQualifiedIdentifierChain(tokenStream, varNode->getName());

            // Parse generic type arguments if present (e.g., Box::method<String, Int>)
            std::vector<std::string> genericTypeArguments;
            if (tokenStream.check(TokenType::LESS))
            {
                tokenStream.advance(); // consume '<'
                // TODO: Delegate to ArgumentParser for generic type arguments
                expectToken(TokenType::GREATER, getParserName());
            }

            // Check if this is a function call (e.g., MathUtils::max(10, 5))
            if (tokenStream.check(TokenType::LPAREN))
            {
                if (!genericTypeArguments.empty() && parts.size() >= 2)
                {
                    return parseGenericMethodCall(parts[0], parts[1], genericTypeArguments);
                }
                else
                {
                    // Regular function call
                    std::string fullName = ParserUtils::buildQualifiedName(parts);
                    tokenStream.advance(); // consume '('
                    std::vector<std::unique_ptr<ASTNode>> arguments;

                    // Parse arguments: arg1, arg2, arg3, ...
                    if (!tokenStream.check(TokenType::RPAREN))
                    {
                        arguments.push_back(context.parseExpression());

                        while (tokenStream.check(TokenType::COMMA))
                        {
                            tokenStream.advance(); // consume ','
                            arguments.push_back(context.parseExpression());
                        }
                    }

                    expectToken(TokenType::RPAREN, getParserName());
                    return std::make_unique<FunctionCallNode>(fullName, std::move(arguments), getCurrentLocation());
                }
            }
            else
            {
                // Qualified variable access (e.g., TestClass::myField)
                std::string fullName = ParserUtils::buildQualifiedName(parts);
                auto qualifiedVarLocation = varNode->getLocation();
                return std::make_unique<VariableNode>(fullName, qualifiedVarLocation);
            }
        }

        reportError("Invalid scope resolution target", getParserName());
        throw errors::ParseException("Invalid scope resolution target");
    }

    std::unique_ptr<ASTNode> PostfixOperatorParser::parseGenericMethodCall(
        const std::string& className,
        const std::string& methodName,
        const std::vector<std::string>& typeArgs)
    {
        // Parse arguments
        expectToken(TokenType::LPAREN, getParserName());
        std::vector<std::unique_ptr<ASTNode>> arguments;

        // Parse arguments: arg1, arg2, arg3, ...
        if (!tokenStream.check(TokenType::RPAREN))
        {
            arguments.push_back(context.parseExpression());

            while (tokenStream.check(TokenType::COMMA))
            {
                tokenStream.advance(); // consume ','
                arguments.push_back(context.parseExpression());
            }
        }

        expectToken(TokenType::RPAREN, getParserName());

        // Create MethodCallNode for static generic call
        auto classNodeLocation = getCurrentLocation();
        auto classNode = std::make_unique<VariableNode>(className, classNodeLocation);
        return std::make_unique<MethodCallNode>(std::move(classNode), methodName, std::move(arguments),
                                              true, typeArgs, classNodeLocation); // isStatic = true
    }

    bool PostfixOperatorParser::isPostfixOperator(TokenType type) const noexcept
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
}