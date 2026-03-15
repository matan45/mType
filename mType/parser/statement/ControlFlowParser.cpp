#include "ControlFlowParser.hpp"
#include "../../ast/nodes/statements/IfNode.hpp"
#include "../../ast/nodes/statements/SwitchNode.hpp"
#include "../../ast/nodes/statements/CaseNode.hpp"
#include "../../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../../ast/nodes/statements/MatchNode.hpp"
#include "../../ast/nodes/statements/MatchCaseNode.hpp"
#include "../../ast/nodes/statements/MatchDefaultNode.hpp"
#include "../../ast/nodes/statements/BlockNode.hpp"
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
        case TokenType::MATCH:
            return parseMatchStatement();
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

    std::unique_ptr<ASTNode> ControlFlowParser::parseMatchStatement()
    {
        auto matchLocation = tokenStream.current().location;
        expectToken(TokenType::MATCH);
        expectToken(TokenType::LPAREN);
        auto expression = context.parseExpression();
        expectToken(TokenType::RPAREN);
        expectToken(TokenType::LBRACE);

        auto matchNode = std::make_unique<MatchNode>(std::move(expression), matchLocation);

        while (!tokenStream.check(TokenType::RBRACE) && !isAtEnd())
        {
            if (tokenStream.check(TokenType::DEFAULT))
            {
                auto defaultLoc = tokenStream.current().location;
                tokenStream.advance(); // consume 'default'
                expectToken(TokenType::ARROW);

                // Parse body: block or single statement
                std::unique_ptr<ASTNode> body;
                if (tokenStream.check(TokenType::LBRACE))
                {
                    body = context.parseStatement(); // block
                }
                else
                {
                    body = context.parseStatement(); // single statement
                }

                matchNode->addCase(std::make_unique<MatchDefaultNode>(std::move(body), defaultLoc));
            }
            else if (tokenStream.check(TokenType::CASE))
            {
                auto caseLoc = tokenStream.current().location;
                tokenStream.advance(); // consume 'case'

                // Check for null pattern
                if (tokenStream.check(TokenType::NULL_LITERAL))
                {
                    tokenStream.advance(); // consume 'null'
                    expectToken(TokenType::ARROW);

                    std::unique_ptr<ASTNode> body;
                    if (tokenStream.check(TokenType::LBRACE))
                    {
                        body = context.parseStatement();
                    }
                    else
                    {
                        body = context.parseStatement();
                    }

                    matchNode->addCase(std::make_unique<MatchCaseNode>(
                        std::move(body), PatternKind::NULL_PATTERN, caseLoc));
                }
                // Check for type pattern: type keyword or uppercase identifier followed by identifier
                else if (isTypePatternStart())
                {
                    std::string typeName = tokenStream.current().stringValue.getString();
                    tokenStream.advance(); // consume type name

                    std::string bindingName = tokenStream.current().stringValue.getString();
                    expectToken(TokenType::IDENTIFIER);
                    expectToken(TokenType::ARROW);

                    std::unique_ptr<ASTNode> body;
                    if (tokenStream.check(TokenType::LBRACE))
                    {
                        body = context.parseStatement();
                    }
                    else
                    {
                        body = context.parseStatement();
                    }

                    matchNode->addCase(std::make_unique<MatchCaseNode>(
                        typeName, bindingName, std::move(body), caseLoc));
                }
                // Value pattern: expression
                else
                {
                    auto valueExpr = context.parseExpression();
                    expectToken(TokenType::ARROW);

                    std::unique_ptr<ASTNode> body;
                    if (tokenStream.check(TokenType::LBRACE))
                    {
                        body = context.parseStatement();
                    }
                    else
                    {
                        body = context.parseStatement();
                    }

                    matchNode->addCase(std::make_unique<MatchCaseNode>(
                        std::move(valueExpr), std::move(body), caseLoc));
                }
            }
            else
            {
                throw ParseException("Expected 'case' or 'default' in match statement",
                                     tokenStream.current().location);
            }
        }

        expectToken(TokenType::RBRACE);
        return std::move(matchNode);
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
        const auto returnLocation = tokenStream.current().location;
        expectToken(TokenType::RETURN);

        std::unique_ptr<ASTNode> value = nullptr;
        if (!tokenStream.check(TokenType::SEMICOLON))
        {
            value = context.parseExpression();
        }

        expectToken(TokenType::SEMICOLON);
        return std::make_unique<ReturnNode>(std::move(value), returnLocation);
    }

    bool ControlFlowParser::isTypePatternStart() const
    {
        auto currentType = tokenStream.current().type;

        // Check if current token is a type keyword
        bool isTypeKeyword = (currentType == TokenType::INT ||
                              currentType == TokenType::FLOAT ||
                              currentType == TokenType::BOOL ||
                              currentType == TokenType::STRING_TYPE);

        // Or an uppercase identifier (class/interface name)
        bool isClassName = (currentType == TokenType::IDENTIFIER &&
                            !tokenStream.current().stringValue.getString().empty() &&
                            std::isupper(tokenStream.current().stringValue.getString()[0]));

        if (!isTypeKeyword && !isClassName) return false;

        // Peek ahead: must be followed by an identifier (the binding name)
        auto nextToken = tokenStream.peekAhead(1);
        return nextToken.type == TokenType::IDENTIFIER;
    }

    bool ControlFlowParser::isControlFlowToken(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::IF:
        case TokenType::SWITCH:
        case TokenType::MATCH:
        case TokenType::BREAK:
        case TokenType::CONTINUE:
        case TokenType::RETURN:
            return true;
        default:
            return false;
        }
    }
}
