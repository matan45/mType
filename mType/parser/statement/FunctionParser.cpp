#include "FunctionParser.hpp"
#include "../TypeParser.hpp"
#include "../ParserUtils.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../ast/GenericType.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::statement
{
    using namespace ast::nodes::functions;
    using namespace token;
    using namespace errors;

    std::unique_ptr<ASTNode> FunctionParser::parse()
    {
        const Token& current = currentToken();

        if (current.type == TokenType::NATIVE)
        {
            return parseNativeFunction();
        }
        else if (current.type == TokenType::FUNCTION)
        {
            return parseFunction();
        }
        else
        {
            reportError("Unexpected token in function parser", getParserName());
            throw errors::ParseException("Invalid function token");
        }
    }

    bool FunctionParser::canParse(const TokenStream& stream) const
    {
        return isFunctionToken(stream.current().type);
    }

    std::unique_ptr<ASTNode> FunctionParser::parseFunction()
    {
        bool isNative = false;

        // Check for native keyword
        if (tryConsumeToken(TokenType::NATIVE))
        {
            isNative = true;
        }

        expectToken(TokenType::FUNCTION, getParserName());

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            reportError("Expected function name", getParserName());
            throw errors::ParseException("Expected function name");
        }

        std::string funcName = tokenStream.current().stringValue.getString();
        validateFunctionName(funcName);
        tokenStream.advance();

        // Use generic-aware parameter parsing to preserve class/interface names
        auto genericParameters = ParserUtils::parseGenericParameterList(tokenStream, true);

        expectToken(TokenType::COLON, getParserName());

        // Parse return type using generic-aware parsing to preserve interface names
        auto genericReturnType = TypeParser::parseGenericType(tokenStream);

        std::unique_ptr<ASTNode> body = nullptr;

        if (isNative)
        {
            // Native functions don't have a body
            expectToken(TokenType::SEMICOLON, getParserName());
        }
        else
        {
            body = context.parseStatement(); // Should be a block
        }

        // Use new generic-aware constructor
        auto funcNode = std::make_unique<FunctionNode>(funcName, genericReturnType,
                                                      genericParameters, std::move(body));
        return funcNode;
    }

    std::unique_ptr<ASTNode> FunctionParser::parseNativeFunction()
    {
        expectToken(TokenType::NATIVE, getParserName());
        expectToken(TokenType::FUNCTION, getParserName());

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            reportError("Expected function name", getParserName());
            throw errors::ParseException("Expected function name");
        }

        std::string funcName = tokenStream.current().stringValue.getString();
        validateFunctionName(funcName);
        tokenStream.advance();

        auto parameters = parseParameterList();

        expectToken(TokenType::COLON, getParserName());
        ValueType returnType = TypeParser::parseType(tokenStream);

        expectToken(TokenType::SEMICOLON, getParserName());

        // Native functions don't have a body - explicitly use unique_ptr nullptr
        return std::make_unique<FunctionNode>(funcName, returnType, std::move(parameters),
                                            std::unique_ptr<ASTNode>(nullptr));
    }

    bool FunctionParser::isFunctionToken(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::FUNCTION:
        case TokenType::NATIVE:
            return true;
        default:
            return false;
        }
    }

    std::vector<std::pair<std::string, ValueType>> FunctionParser::parseParameterList()
    {
        // Delegate to centralized utility
        return ParserUtils::parseParameterList(tokenStream, true);
    }

    void FunctionParser::validateFunctionName(const std::string& funcName)
    {
        try
        {
            // Validate function naming convention (must start with lowercase)
            ParserUtils::validateFunctionNamingConvention(funcName, false, "Function", tokenStream.location());
        }
        catch (const std::exception& e)
        {
            reportError(e.what(), getParserName());
            throw;
        }
    }
}