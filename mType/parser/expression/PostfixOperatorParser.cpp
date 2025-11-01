#include "PostfixOperatorParser.hpp"
#include "ArgumentParser.hpp"
#include "../ExpressionParser.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../ast/nodes/expressions/VariableNode.hpp"
#include "../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::expression
{
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::classes;
    using namespace token;
    using namespace errors;

    PostfixOperatorParser::PostfixOperatorParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx), expressionParser(nullptr)
    {
    }

    void PostfixOperatorParser::setExpressionParser(ExpressionParser& exprParser)
    {
        expressionParser = &exprParser;
    }

    std::unique_ptr<ASTNode> PostfixOperatorParser::parse()
    {
        throw ParseException("Invalid use of PostfixOperatorParser", tokenStream.current().location);
    }

    std::unique_ptr<ASTNode> PostfixOperatorParser::parsePostfix()
    {
        if (!expressionParser)
        {
            throw ParseException("ExpressionParser not initialized in PostfixOperatorParser",
                                 tokenStream.current().location);
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
                SourceLocation opLocation = tokenStream.current().location; // Capture location before advancing
                tokenStream.advance();
                expr = std::make_unique<UnaryExpNode>(op, std::move(expr), UnaryPosition::POSTFIX, opLocation);
            }
            else if (tokenStream.check(TokenType::LESS) && dynamic_cast<VariableNode*>(expr.get()) &&
                isGenericFunctionCall())
            {
                // Generic function call: identifier<Type>(args)
                expr = parseFunctionCall(std::move(expr));
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
            // Parse generic type arguments if present (e.g., identity<Int>)
            std::vector<std::string> genericTypeArguments;
            if (tokenStream.check(TokenType::LESS))
            {
                tokenStream.advance(); // consume '<'
                if (!expressionParser)
                {
                    throw ParseException("ExpressionParser not initialized in PostfixOperatorParser",
                                         tokenStream.current().location);
                }
                genericTypeArguments = expressionParser->parseGenericTypeArguments();
                expectToken(TokenType::GREATER);
            }

            ArgumentParser argParser(tokenStream, context);
            std::vector<std::unique_ptr<ASTNode>> arguments = argParser.parseArgumentsWithParentheses();
            return std::make_unique<FunctionCallNode>(funcName, std::move(arguments), genericTypeArguments,
                                                      tokenStream.current().location);
        }

        throw ParseException("Invalid function call target", tokenStream.current().location);
    }

    std::unique_ptr<ASTNode> PostfixOperatorParser::parseMemberAccess(std::unique_ptr<ASTNode> object)
    {
        expectToken(TokenType::DOT);

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            throw ParseException("Expected member name after '.'", tokenStream.current().location);
        }

        std::string memberName = tokenStream.current().stringValue.getString();
        SourceLocation location = tokenStream.current().location;
        tokenStream.advance();

        // Parse generic type arguments if present (e.g., obj.method<String, Int>)
        std::vector<std::string> genericTypeArguments;
        if (tokenStream.check(TokenType::LESS))
        {
            tokenStream.advance(); // consume '<'
            if (!expressionParser)
            {
                throw ParseException("ExpressionParser not initialized in PostfixOperatorParser",
                                     tokenStream.current().location);
            }
            genericTypeArguments = expressionParser->parseGenericTypeArguments();
            expectToken(TokenType::GREATER);
        }

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

            expectToken(TokenType::RPAREN);
            return std::make_unique<MethodCallNode>(std::move(object), memberName, std::move(arguments),
                                                    false, genericTypeArguments, location);
        }

        return std::make_unique<MemberAccessNode>(std::move(object), memberName, false, location);
    }

    std::unique_ptr<ASTNode> PostfixOperatorParser::parseIndexAccess(std::unique_ptr<ASTNode> collection)
    {
        SourceLocation location = tokenStream.current().location;
        expectToken(TokenType::LBRACKET);

        auto index = context.parseExpression();
        expectToken(TokenType::RBRACKET);

        return std::make_unique<IndexAccessNode>(std::move(collection), std::move(index), location);
    }

    std::unique_ptr<ASTNode> PostfixOperatorParser::parseScopeResolution(std::unique_ptr<ASTNode> expr)
    {
        expectToken(TokenType::SCOPE);

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            throw ParseException("Expected identifier after '::'", tokenStream.current().location);
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
                if (!expressionParser)
                {
                    throw ParseException("ExpressionParser not initialized in PostfixOperatorParser",
                                         tokenStream.current().location);
                }
                genericTypeArguments = expressionParser->parseGenericTypeArguments();
                expectToken(TokenType::GREATER);
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
                    ArgumentParser argParser(tokenStream, context);
                    std::vector<std::unique_ptr<ASTNode>> arguments = argParser.parseArgumentsWithParentheses();
                    return std::make_unique<FunctionCallNode>(fullName, std::move(arguments),
                                                              tokenStream.current().location);
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

        throw ParseException("Invalid scope resolution target", tokenStream.current().location);
    }

    std::unique_ptr<ASTNode> PostfixOperatorParser::parseGenericMethodCall(
        const std::string& className,
        const std::string& methodName,
        const std::vector<std::string>& typeArgs)
    {
        // Parse arguments
        expectToken(TokenType::LPAREN);
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

        expectToken(TokenType::RPAREN);

        // Create MethodCallNode for static generic call
        auto classNodeLocation = tokenStream.current().location;
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

    bool PostfixOperatorParser::isGenericFunctionCall()
    {
        // Lookahead to determine if identifier< is a generic function call or comparison
        // Generic function call pattern: identifier<Type1, Type2>(args)
        // We need to check if after the generic type args there's a '('

        // Should be at '<'
        if (!tokenStream.check(TokenType::LESS))
        {
            return false;
        }

        try
        {
            // Use peek to lookahead without modifying state
            size_t offset = 1; // Start after '<'
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
                        // Found matching '>', now check for '('
                        Token afterGreater = tokenStream.peekAhead(offset + 1);
                        return afterGreater.type == TokenType::LPAREN;
                    }
                }
                else if (nextToken.type != TokenType::IDENTIFIER &&
                    nextToken.type != TokenType::COMMA)
                {
                    // Invalid token for generic type argument
                    return false;
                }

                offset++;

                // Safety check to avoid infinite loop
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
}
