#include "StatementParser.hpp"
#include "TypeParser.hpp"
#include "utilities/ParserUtils.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace ast::nodes::statements;
    using namespace token;
    using namespace errors;

    void StatementParser::initializeHelperParsers()
    {
        controlFlowParser = std::make_unique<ControlFlowParser>(tokenStream, context);
        loopParser = std::make_unique<LoopParser>(tokenStream, context);
        declarationParser = std::make_unique<DeclarationParser>(tokenStream, context);
        assignmentParser = std::make_unique<AssignmentStatementParser>(tokenStream, context);
        functionParser = std::make_unique<FunctionParser>(tokenStream, context);
        importParser = std::make_unique<ImportParser>(tokenStream, context);
        exceptionParser = std::make_unique<ExceptionParser>(tokenStream, context);
    }

    StatementParser::StatementParser(TokenStream& stream, ParseContext& ctx)
        : tokenStream(stream), context(ctx), expressionParser(nullptr)
    {
        initializeHelperParsers();
    }

    std::unique_ptr<ASTNode> StatementParser::parseStatement()
    {
        StatementType type;
        try
        {
            type = StatementTypeDetector::detectStatementType(tokenStream);
        }
        catch (const std::exception&)
        {
            throw;
        }

        switch (type)
        {
        case StatementType::CLASS:
            return context.parseClass();
        case StatementType::INTERFACE:
            return context.parseInterface();
        case StatementType::BLOCK:
            return parseBlock();
        case StatementType::EMPTY:
            tokenStream.advance(); // Skip empty statement
            return nullptr;
        default:
            return delegateToSpecializedParser(type);
        }
    }

    std::unique_ptr<ASTNode> StatementParser::parseBlock()
    {
        auto blockLocation = tokenStream.current().location; // Capture location before expecting LBRACE
        tokenStream.expect(TokenType::LBRACE);
        auto block = std::make_unique<BlockNode>(blockLocation);

        while (!tokenStream.check(TokenType::RBRACE) && !tokenStream.isAtEnd())
        {
            auto statement = parseStatement();
            if (statement)
            {
                block->addStatement(std::move(statement));
            }
        }

        tokenStream.expect(TokenType::RBRACE);
        return std::move(block);
    }

    std::unique_ptr<ASTNode> StatementParser::delegateToSpecializedParser(StatementType type)
    {
        try
        {
            switch (type)
            {
            case StatementType::CONTROL_FLOW:
                return controlFlowParser->parse();
            case StatementType::LOOP:
                return loopParser->parse();
            case StatementType::DECLARATION:
                return declarationParser->parse();
            case StatementType::ASSIGNMENT:
                return assignmentParser->parseAssignment();
            case StatementType::FUNCTION:
                return functionParser->parse();
            case StatementType::IMPORT:
                return importParser->parse();
            case StatementType::EXCEPTION:
                return exceptionParser->parse();
            case StatementType::EXPRESSION:
                return assignmentParser->parseExpressionStatement();
            default:
                throw ParseException("Unknown statement type", tokenStream.current().location);
            }
        }
        catch (const ParseException&)
        {
            throw;
        }
        catch (const std::exception&)
        {
            throw ParseException("Parser error during statement processing", tokenStream.current().location);
        }
    }

    // Delegated methods for backward compatibility
    std::unique_ptr<ASTNode> StatementParser::parseDeclaration()
    {
        return declarationParser->parseDeclaration();
    }

    std::unique_ptr<ASTNode> StatementParser::parseAssignment()
    {
        return assignmentParser->parseAssignment();
    }

    std::unique_ptr<ASTNode> StatementParser::parseIfStatement()
    {
        return controlFlowParser->parseIfStatement();
    }

    std::unique_ptr<ASTNode> StatementParser::parseWhileStatement()
    {
        return loopParser->parseWhileStatement();
    }

    std::unique_ptr<ASTNode> StatementParser::parseDoWhileStatement()
    {
        return loopParser->parseDoWhileStatement();
    }

    std::unique_ptr<ASTNode> StatementParser::parseForStatement()
    {
        return loopParser->parseForStatement();
    }

    std::unique_ptr<ASTNode> StatementParser::parseForEachStatement()
    {
        return loopParser->parseForEachStatement();
    }

    std::unique_ptr<ASTNode> StatementParser::parseSwitchStatement()
    {
        return controlFlowParser->parseSwitchStatement();
    }

    std::unique_ptr<ASTNode> StatementParser::parseBreakStatement()
    {
        return controlFlowParser->parseBreakStatement();
    }

    std::unique_ptr<ASTNode> StatementParser::parseContinueStatement()
    {
        return controlFlowParser->parseContinueStatement();
    }

    std::unique_ptr<ASTNode> StatementParser::parseReturnStatement()
    {
        return controlFlowParser->parseReturnStatement();
    }

    std::unique_ptr<ASTNode> StatementParser::parseExpressionStatement()
    {
        return assignmentParser->parseExpressionStatement();
    }

    std::unique_ptr<ASTNode> StatementParser::parseFunction()
    {
        return functionParser->parseFunction();
    }

    std::unique_ptr<ASTNode> StatementParser::parseImport()
    {
        return importParser->parseImport();
    }

    std::unique_ptr<ASTNode> StatementParser::parseNativeFunction()
    {
        return functionParser->parseNativeFunction();
    }

    std::unique_ptr<ASTNode> StatementParser::parseTryStatement()
    {
        return exceptionParser->parseTryStatement();
    }

    std::unique_ptr<ASTNode> StatementParser::parseThrowStatement()
    {
        return exceptionParser->parseThrowStatement();
    }

}
