#include "StatementParser.hpp"
#include "TypeParser.hpp"
#include "ParserUtils.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../exceptions/DomainExceptions.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace ast::nodes::statements;
    using namespace token;
    using namespace errors;

    void StatementParser::initializeHelperParsers()
    {
        controlFlowParser = std::make_unique<ControlFlowParser>(tokenStream, context, errorHandler);
        loopParser = std::make_unique<LoopParser>(tokenStream, context, errorHandler);
        declarationParser = std::make_unique<DeclarationParser>(tokenStream, context, errorHandler);
        assignmentParser = std::make_unique<AssignmentStatementParser>(tokenStream, context, errorHandler);
        functionParser = std::make_unique<FunctionParser>(tokenStream, context, errorHandler);
        importParser = std::make_unique<ImportParser>(tokenStream, context, errorHandler);
    }

    std::unique_ptr<ASTNode> StatementParser::parseStatement()
    {
        StatementType type = StatementTypeDetector::detectStatementType(tokenStream);

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
            case StatementType::EXPRESSION:
                return assignmentParser->parseExpressionStatement();
            default:
                throw errors::ParseException("Unknown statement type");
            }
        }
        catch (const errors::ParseException& e)
        {
            errorHandler->reportError(error::ErrorContext(tokenStream.current().location,
                                                        std::string("Error in StatementParser: ") + e.what()));
            throw;
        }
        catch (const std::exception& e)
        {
            errorHandler->reportError(error::ErrorContext(tokenStream.current().location,
                                                        std::string("Unexpected error in StatementParser: ") + e.what()));
            throw errors::ParseException("Parser error during statement processing");
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

    // Helper methods that delegate to ParserUtils
    std::vector<std::pair<std::string, ValueType>> StatementParser::parseParameterList()
    {
        return ParserUtils::parseParameterList(tokenStream, true);
    }

    std::unique_ptr<ASTNode> StatementParser::tryParseForEach()
    {
        // This is handled by the LoopParser now
        return loopParser->parseForEachStatement();
    }
}