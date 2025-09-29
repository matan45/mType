#include "ControlFlowParser.hpp"
#include "../../ast/nodes/statements/IfNode.hpp"
#include "../../ast/nodes/statements/SwitchNode.hpp"
#include "../../ast/nodes/statements/CaseNode.hpp"
#include "../../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../../ast/nodes/statements/BreakNode.hpp"
#include "../../ast/nodes/statements/ContinueNode.hpp"
#include "../../ast/nodes/functions/ReturnNode.hpp"
#include "../../exceptions/DomainExceptions.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::statement
{
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;
    using namespace token;

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
            reportError("Unexpected token in control flow parser", getParserName());
            throw errors::ParseException("Invalid control flow token");
        }
    }

    bool ControlFlowParser::canParse(const TokenStream& stream) const
    {
        return isControlFlowToken(stream.current().type);
    }

    std::unique_ptr<ASTNode> ControlFlowParser::parseIfStatement()
    {
        expectToken(TokenType::IF, getParserName());
        expectToken(TokenType::LPAREN, getParserName());
        auto condition = context.parseExpression();
        expectToken(TokenType::RPAREN, getParserName());

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
        expectToken(TokenType::SWITCH, getParserName());
        expectToken(TokenType::LPAREN, getParserName());
        auto expression = context.parseExpression();
        expectToken(TokenType::RPAREN, getParserName());
        expectToken(TokenType::LBRACE, getParserName());

        auto switchNode = std::make_unique<SwitchNode>(std::move(expression));

        while (!tokenStream.check(TokenType::RBRACE) && !isAtEnd())
        {
            if (tokenStream.check(TokenType::CASE))
            {
                tokenStream.advance();
                auto caseValue = context.parseExpression();
                expectToken(TokenType::COLON, getParserName());

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
                auto defaultLocation = getCurrentLocation();
                tokenStream.advance();
                expectToken(TokenType::COLON, getParserName());

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
                reportWarning("Unexpected token in switch statement, skipping", getParserName());
                tokenStream.advance();
            }
        }

        expectToken(TokenType::RBRACE, getParserName());
        return std::move(switchNode);
    }

    std::unique_ptr<ASTNode> ControlFlowParser::parseBreakStatement()
    {
        auto breakLocation = getCurrentLocation();
        expectToken(TokenType::BREAK, getParserName());
        expectToken(TokenType::SEMICOLON, getParserName());
        return std::make_unique<BreakNode>(breakLocation);
    }

    std::unique_ptr<ASTNode> ControlFlowParser::parseContinueStatement()
    {
        auto continueLocation = getCurrentLocation();
        expectToken(TokenType::CONTINUE, getParserName());
        expectToken(TokenType::SEMICOLON, getParserName());
        return std::make_unique<ContinueNode>(continueLocation);
    }

    std::unique_ptr<ASTNode> ControlFlowParser::parseReturnStatement()
    {
        expectToken(TokenType::RETURN, getParserName());

        std::unique_ptr<ASTNode> value = nullptr;
        if (!tokenStream.check(TokenType::SEMICOLON))
        {
            value = context.parseExpression();
        }

        expectToken(TokenType::SEMICOLON, getParserName());
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