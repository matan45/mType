#include "ExceptionParser.hpp"
#include "../../ast/nodes/statements/TryNode.hpp"
#include "../../ast/nodes/statements/CatchNode.hpp"
#include "../../ast/nodes/statements/ThrowNode.hpp"
#include "../../errors/ParseException.hpp"
#include "../TypeParser.hpp"
#include <unordered_set>

namespace parser::statement
{
    using namespace ast::nodes::statements;
    using namespace token;
    using namespace errors;

    ExceptionParser::ExceptionParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }

    std::unique_ptr<ASTNode> ExceptionParser::parse()
    {
        const Token& current = currentToken();

        switch (current.type)
        {
        case TokenType::TRY:
            return parseTryStatement();
        case TokenType::THROW:
            return parseThrowStatement();
        default:
            throw ParseException("Invalid exception handling token", tokenStream.current().location);
        }
    }

    bool ExceptionParser::canParse(const TokenStream& stream) const
    {
        return isExceptionToken(stream.current().type);
    }

    std::unique_ptr<TryNode> ExceptionParser::parseTryStatement()
    {
        auto tryLocation = tokenStream.current().location;
        expectToken(TokenType::TRY);

        // Parse try block
        auto tryBlock = context.parseStatement();

        // Parse catch blocks
        std::vector<std::unique_ptr<CatchNode>> catchBlocks;
        std::unordered_set<std::string> seenCatchTypes;

        while (tokenStream.check(TokenType::CATCH))
        {
            auto catchBlock = parseCatchClause();
            const std::string& exceptionType = catchBlock->getExceptionType();

            // Check for duplicate catch type
            if (seenCatchTypes.find(exceptionType) != seenCatchTypes.end())
            {
                throw ParseException(
                    "Duplicate catch block for exception type '" + exceptionType + "'",
                    tokenStream.current().location
                );
            }

            seenCatchTypes.insert(exceptionType);
            catchBlocks.push_back(std::move(catchBlock));
        }

        // Parse optional finally block
        std::unique_ptr<ASTNode> finallyBlock = nullptr;
        if (tryConsumeToken(TokenType::FINALLY))
        {
            finallyBlock = parseFinallyClause();
        }

        // Validate that we have at least one catch or finally
        if (catchBlocks.empty() && !finallyBlock)
        {
            throw ParseException("try statement must have at least one catch or finally clause", tryLocation);
        }

        return std::make_unique<TryNode>(
            std::move(tryBlock),
            std::move(catchBlocks),
            std::move(finallyBlock),
            tryLocation
        );
    }

    std::unique_ptr<CatchNode> ExceptionParser::parseCatchClause()
    {
        auto catchLocation = tokenStream.current().location;
        expectToken(TokenType::CATCH);
        expectToken(TokenType::LPAREN);

        // Parse exception type (supports generics like ResultException<Int>)
        auto exceptionType = TypeParser::parseGenericType(tokenStream);

        // Convert GenericType to string representation for CatchNode
        std::string exceptionTypeStr = exceptionType->toString();

        // Parse variable name
        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            throw ParseException("Expected variable name in catch clause", tokenStream.current().location);
        }
        std::string variableName = tokenStream.current().stringValue.getString();
        tokenStream.advance();

        expectToken(TokenType::RPAREN);

        // Parse catch body
        auto body = context.parseStatement();

        return std::make_unique<CatchNode>(
            exceptionTypeStr,
            variableName,
            std::move(body),
            catchLocation
        );
    }

    std::unique_ptr<ASTNode> ExceptionParser::parseFinallyClause()
    {
        // FINALLY token already consumed by caller
        return context.parseStatement();
    }

    std::unique_ptr<ThrowNode> ExceptionParser::parseThrowStatement()
    {
        auto throwLocation = tokenStream.current().location;
        expectToken(TokenType::THROW);

        // Parse expression to throw
        auto expression = context.parseExpression();

        expectToken(TokenType::SEMICOLON);

        return std::make_unique<ThrowNode>(std::move(expression), throwLocation);
    }

    bool ExceptionParser::isExceptionToken(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::TRY:
        case TokenType::THROW:
            return true;
        default:
            return false;
        }
    }
}
