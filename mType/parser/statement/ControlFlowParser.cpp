#include "ControlFlowParser.hpp"
#include "../../ast/nodes/statements/IfNode.hpp"
#include "../../ast/nodes/statements/SwitchNode.hpp"
#include "../../ast/nodes/statements/CaseNode.hpp"
#include "../../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../../ast/nodes/statements/BreakNode.hpp"
#include "../../ast/nodes/statements/ContinueNode.hpp"
#include "../../ast/nodes/functions/ReturnNode.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::statement
{
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;
    using namespace token;

    ControlFlowParser::ControlFlowParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }

    std::unique_ptr<ASTNode> ControlFlowParser::parse()
    {
        const Token& current = currentToken();

        switch (current.type)
        {
        case TokenType::IF:
            return parseIfStatement();
        case TokenType::SWITCH:
            return parseSwitchStatement();
        case TokenType::BREAK:
            return parseBreakStatement();
        case TokenType::CONTINUE:
            return parseContinueStatement();
        case TokenType::RETURN:
            return parseReturnStatement();
        default:
            throw ParseException("Invalid control flow token", tokenStream.current().location);
        }
    }

    bool ControlFlowParser::canParse(const TokenStream& stream) const
    {
        return isControlFlowToken(stream.current().type);
    }

    std::unique_ptr<ASTNode> ControlFlowParser::parseIfStatement()
    {
        expectToken(TokenType::IF);
        expectToken(TokenType::LPAREN);
        auto condition = context.parseExpression();
        expectToken(TokenType::RPAREN);

        auto thenStatement = context.parseStatement();

        std::unique_ptr<ASTNode> elseStatement = nullptr;
        if (tryConsumeToken(TokenType::ELSE))
        {
            elseStatement = context.parseStatement();
        }

        return std::make_unique<IfNode>(std::move(condition), std::move(thenStatement), std::move(elseStatement));
    }

    std::unique_ptr<ASTNode> ControlFlowParser::parseSwitchStatement()
    {
        expectToken(TokenType::SWITCH);
        expectToken(TokenType::LPAREN);
        auto expression = context.parseExpression();
        expectToken(TokenType::RPAREN);
        expectToken(TokenType::LBRACE);

        auto switchNode = std::make_unique<SwitchNode>(std::move(expression));

        while (!tokenStream.check(TokenType::RBRACE) && !isAtEnd())
        {
            if (tokenStream.check(TokenType::CASE))
            {
                tokenStream.advance();
                auto caseValue = context.parseExpression();
                expectToken(TokenType::COLON);

                auto caseNode = std::make_unique<CaseNode>(std::move(caseValue));
                while (!tokenStream.check(TokenType::CASE) &&
                    !tokenStream.check(TokenType::DEFAULT) &&
                    !tokenStream.check(TokenType::RBRACE) &&
                    !isAtEnd())
                {
                    auto stmt = context.parseStatement();
                    if (stmt)
                    {
                        caseNode->addStatement(std::move(stmt));
                    }
                }

                switchNode->addCase(std::move(caseNode));
            }
            else if (tokenStream.check(TokenType::DEFAULT))
            {
                auto defaultLocation = tokenStream.current().location;
                tokenStream.advance();
                expectToken(TokenType::COLON);

                auto defaultNode = std::make_unique<DefaultCaseNode>(defaultLocation);
                while (!tokenStream.check(TokenType::CASE) &&
                    !tokenStream.check(TokenType::DEFAULT) &&
                    !tokenStream.check(TokenType::RBRACE) &&
                    !isAtEnd())
                {
                    auto stmt = context.parseStatement();
                    if (stmt)
                    {
                        defaultNode->addStatement(std::move(stmt));
                    }
                }

                switchNode->addCase(std::move(defaultNode));
            }
            else
            {
                tokenStream.advance();
            }
        }

        expectToken(TokenType::RBRACE);
        return std::move(switchNode);
    }

    std::unique_ptr<ASTNode> ControlFlowParser::parseBreakStatement()
    {
        auto breakLocation = tokenStream.current().location;
        expectToken(TokenType::BREAK);
        expectToken(TokenType::SEMICOLON);
        return std::make_unique<BreakNode>(breakLocation);
    }

    std::unique_ptr<ASTNode> ControlFlowParser::parseContinueStatement()
    {
        expectToken(TokenType::CONTINUE);
        expectToken(TokenType::SEMICOLON);
        return std::make_unique<ContinueNode>(tokenStream.current().location);
    }

    std::unique_ptr<ASTNode> ControlFlowParser::parseReturnStatement()
    {
        expectToken(TokenType::RETURN);

        std::unique_ptr<ASTNode> value = nullptr;
        if (!tokenStream.check(TokenType::SEMICOLON))
        {
            value = context.parseExpression();
        }

        expectToken(TokenType::SEMICOLON);
        return std::make_unique<ReturnNode>(std::move(value));
    }

    bool ControlFlowParser::isControlFlowToken(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::IF:
        case TokenType::SWITCH:
        case TokenType::BREAK:
        case TokenType::CONTINUE:
        case TokenType::RETURN:
            return true;
        default:
            return false;
        }
    }
}
