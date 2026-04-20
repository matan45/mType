#include "LoopParser.hpp"
#include "../TypeParser.hpp"
#include "../../ast/nodes/statements/WhileNode.hpp"
#include "../../ast/nodes/statements/DoWhileNode.hpp"
#include "../../ast/nodes/statements/ForNode.hpp"
#include "../../ast/nodes/statements/ForEachNode.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::statement
{
    using namespace ast::nodes::statements;
    using namespace token;
    using namespace errors;

    LoopParser::LoopParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }

    std::unique_ptr<ASTNode> LoopParser::parse()
    {
        const Token& current = currentToken();

        switch (current.type)
        {
        case TokenType::WHILE:
            return parseWhileStatement();
        case TokenType::DO:
            return parseDoWhileStatement();
        case TokenType::FOR:
            return parseForStatement();
        default:
            throw ParseException("Invalid loop token", tokenStream.current().location);
        }
    }

    bool LoopParser::canParse(const TokenStream& stream) const
    {
        return isLoopToken(stream.current().type);
    }

    std::unique_ptr<ASTNode> LoopParser::parseWhileStatement()
    {
        expectToken(TokenType::WHILE);
        expectToken(TokenType::LPAREN);
        auto condition = context.parseExpression();
        expectToken(TokenType::RPAREN);

        auto body = context.parseStatement();
        return std::make_unique<WhileNode>(std::move(condition), std::move(body));
    }

    std::unique_ptr<ASTNode> LoopParser::parseDoWhileStatement()
    {
        expectToken(TokenType::DO);
        auto body = context.parseStatement();
        expectToken(TokenType::WHILE);
        expectToken(TokenType::LPAREN);
        auto condition = context.parseExpression();
        expectToken(TokenType::RPAREN);
        expectToken(TokenType::SEMICOLON);

        return std::make_unique<DoWhileNode>(std::move(body), std::move(condition));
    }

    std::unique_ptr<ASTNode> LoopParser::parseForStatement()
    {
        expectToken(TokenType::FOR);
        expectToken(TokenType::LPAREN);

        // Parse initialization - could be for-each or regular for loop
        std::unique_ptr<ASTNode> init = nullptr;
        if (!tokenStream.check(TokenType::SEMICOLON))
        {
            if (tokenStream.check(TokenType::INT) ||
                tokenStream.check(TokenType::FLOAT) ||
                tokenStream.check(TokenType::BOOL) ||
                tokenStream.check(TokenType::STRING_TYPE) ||
                tokenStream.check(TokenType::IDENTIFIER))
            {
                // Parse type and variable name
                TypeInfo typeInfo = TypeParser::parseTypeInfo(tokenStream);

                if (!tokenStream.check(TokenType::IDENTIFIER))
                {
                    throw ParseException("Expected variable name", tokenStream.current().location);
                }

                std::string varName = std::string(tokenStream.current().stringValue);
                SourceLocation location = tokenStream.current().location;
                tokenStream.advance();

                // Check if this is for-each (colon) or regular for loop (assignment/semicolon)
                if (tokenStream.check(TokenType::COLON))
                {
                    // This is a for-each loop!
                    tokenStream.advance(); // consume ':'

                    auto collection = context.parseExpression();
                    expectToken(TokenType::RPAREN);

                    auto body = context.parseStatement();

                    return std::make_unique<ForEachNode>(varName, typeInfo,
                                                         std::move(collection), std::move(body), location);
                }
                else
                {
                    // Regular for loop with variable declaration
                    std::unique_ptr<ASTNode> value = nullptr;
                    if (tryConsumeToken(TokenType::ASSIGN))
                    {
                        value = context.parseExpression();
                    }

                    init = std::make_unique<AssignmentNode>(varName, std::move(value),
                                                            typeInfo.baseType, typeInfo.className,
                                                            false, false, location);
                }
            }
            else
            {
                init = context.parseExpression();
            }
        }

        expectToken(TokenType::SEMICOLON);

        // Parse condition
        std::unique_ptr<ASTNode> condition = nullptr;
        if (!tokenStream.check(TokenType::SEMICOLON))
        {
            condition = context.parseExpression();
        }
        expectToken(TokenType::SEMICOLON);

        // Parse update
        std::unique_ptr<ASTNode> update = nullptr;
        if (!tokenStream.check(TokenType::RPAREN))
        {
            update = context.parseExpression();
        }
        expectToken(TokenType::RPAREN);

        auto body = context.parseStatement();

        return std::make_unique<ForNode>(std::move(init), std::move(condition),
                                         std::move(update), std::move(body));
    }

    std::unique_ptr<ASTNode> LoopParser::parseForEachStatement()
    {
        // For-each logic is integrated into parseForStatement()
        // This is a wrapper for backward compatibility
        return parseForStatement();
    }

    bool LoopParser::isLoopToken(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::WHILE:
        case TokenType::DO:
        case TokenType::FOR:
            return true;
        default:
            return false;
        }
    }
}
