#include "StatementParser.hpp"
namespace parser
{
    std::unique_ptr<ASTNode> StatementParser::parseStatement()
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseBlock()
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseDeclaration()
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseAssignment()
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseIfStatement()
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseWhileStatement()
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseDoWhileStatement()
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseForStatement()
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseSwitchStatement()
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseBreakStatement()
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseContinueStatement()
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseReturnStatement()
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseExpressionStatement()
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseFunction()
    {
        return nullptr;
    }

    std::unique_ptr<ASTNode> StatementParser::parseImport()
    {
        return nullptr;
    }

    ValueType StatementParser::parseType()
    {
        return ValueType::VOID;
    }

    std::vector<std::pair<std::string, ValueType>> StatementParser::parseParameterList()
    {
        return std::vector<std::pair<std::string, ValueType>>();
    }
}
