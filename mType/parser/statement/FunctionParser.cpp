#include "FunctionParser.hpp"
#include "../TypeParser.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../utilities/AsyncValidator.hpp"
#include "../utilities/VisibilityParser.hpp"
#include "../utilities/AnnotationParser.hpp"
#include "../class/GenericParameterParser.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../ast/GenericType.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::statement
{
    using namespace ast::nodes::functions;
    using namespace token;
    using namespace errors;
    using namespace parser::utilities;

    FunctionParser::FunctionParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }


    std::unique_ptr<ASTNode> FunctionParser::parse()
    {
        const Token& current = currentToken();

        // Handle annotations (e.g., @Throw)
        if (current.type == TokenType::AT)
        {
            // Annotations are handled in parseFunction()
            return parseFunction();
        }
        // Handle visibility modifiers (public/private)
        else if (current.type == TokenType::PUBLIC || current.type == TokenType::PRIVATE)
        {
            // Visibility modifiers are handled in parseFunction()
            return parseFunction();
        }
        else if (current.type == TokenType::NATIVE)
        {
            return parseNativeFunction();
        }
        else if (current.type == TokenType::FUNCTION)
        {
            return parseFunction();
        }

        throw errors::ParseException("Invalid function token", tokenStream.current().location);
    }

    bool FunctionParser::canParse(const TokenStream& stream) const
    {
        return isFunctionToken(stream.current().type);
    }

    std::unique_ptr<ASTNode> FunctionParser::parseFunction()
    {
        // Step 1: Parse annotations before function declaration (e.g., @Throw(IOException))
        auto annotations = utilities::AnnotationParser::parseAnnotations(tokenStream);

        // Validate: functions cannot be declared inside other functions
        if (context.isInsideFunctionBody())
        {
            throw ParseException("Function declarations inside function bodies are not allowed. "
                                 "Functions must be declared at the top level or inside classes.",
                                 tokenStream.current().location);
        }

        // Parse optional visibility modifier (public/private)
        // Default is PUBLIC if not specified
        VisibilityModifier visibility = VisibilityParser::parseVisibilityModifier(tokenStream);

        bool isNative = false;
        bool isAsync = false;

        // Check for native keyword
        if (tryConsumeToken(TokenType::NATIVE))
        {
            isNative = true;
        }

        expectToken(TokenType::FUNCTION);

        // NEW: Check for async keyword AFTER function keyword
        if (tryConsumeToken(TokenType::ASYNC))
        {
            isAsync = true;
        }

        // Parse generic type parameters (like methods do)
        std::vector<GenericTypeParameter> functionGenericParameters;
        if (tokenStream.check(TokenType::LESS))
        {
            tokenStream.advance(); // consume '<'
            GenericParameterParser genericParser(tokenStream, context);
            functionGenericParameters = genericParser.parseGenericTypeParameters();
            tokenStream.expect(TokenType::GREATER); // consume '>'
        }

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            throw ParseException("Expected function name", tokenStream.current().location);
        }

        std::string funcName = tokenStream.current().stringValue.getString();
        auto funcLocation = tokenStream.current().location;
        validateFunctionName(funcName);
        tokenStream.advance();

        // NEW: Check for duplicate global function name (only for global functions, not class methods)
        if (!context.isInsideClassBody())
        {
            if (context.isFunctionDeclared(funcName))
            {
                throw ParseException(
                    "Duplicate function declaration: '" + funcName + "' has already been declared",
                    funcLocation
                );
            }

            // Register the function name
            context.registerFunctionName(funcName);
        }

        // Use generic-aware parameter parsing to preserve class/interface names
        auto genericParameters = ParserUtils::parseGenericParameterList(tokenStream, true);

        expectToken(TokenType::COLON);

        // Parse return type using generic-aware parsing to preserve interface names
        auto genericReturnType = TypeParser::parseGenericType(tokenStream);

        // Validate async function return type must be Promise<T>
        if (isAsync)
        {
            utilities::AsyncValidator::validateAsyncReturnType(genericReturnType, tokenStream.location());
        }

        std::unique_ptr<ASTNode> body = nullptr;

        if (isNative)
        {
            // Native functions don't have a body
            expectToken(TokenType::SEMICOLON);
        }
        else
        {
            // Set function and async context when parsing function body
            ParseContext::FunctionContextGuard functionGuard(context.getContextState());
            ParseContext::AsyncContextGuard asyncGuard(context.getContextState(), isAsync);
            body = context.parseStatement(); // Should be a block
        }

        // Use new generic-aware constructor with function generic parameters and async flag
        auto funcNode = std::make_unique<FunctionNode>(funcName, genericReturnType,
                                                       genericParameters, std::move(body),
                                                       functionGenericParameters, isAsync, funcLocation);
        funcNode->setVisibility(visibility);

        // Step 2: Add parsed annotations to the function node
        for (auto& annotation : annotations)
        {
            funcNode->addAnnotation(annotation);
        }

        return funcNode;
    }

    std::unique_ptr<ASTNode> FunctionParser::parseNativeFunction()
    {
        expectToken(TokenType::NATIVE);
        expectToken(TokenType::FUNCTION);

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            throw ParseException("Expected function name", tokenStream.current().location);
        }

        std::string funcName = tokenStream.current().stringValue.getString();
        auto funcLocation = tokenStream.current().location;
        validateFunctionName(funcName);
        tokenStream.advance();

        // NEW: Check for duplicate global function name
        if (context.isFunctionDeclared(funcName))
        {
            throw ParseException(
                "Duplicate function declaration: '" + funcName + "' has already been declared",
                funcLocation
            );
        }

        // Register the function name
        context.registerFunctionName(funcName);

        auto parameters = parseParameterList();

        expectToken(TokenType::COLON);
        ValueType returnType = TypeParser::parseType(tokenStream);

        expectToken(TokenType::SEMICOLON);

        // Native functions don't have a body - explicitly use unique_ptr nullptr
        return std::make_unique<FunctionNode>(funcName, returnType, std::move(parameters),
                                              std::unique_ptr<ASTNode>(nullptr), false, funcLocation);
    }

    bool FunctionParser::isFunctionToken(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::AT:        // Annotations like @Throw
        case TokenType::PUBLIC:    // Visibility modifiers
        case TokenType::PRIVATE:
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
        catch (const std::exception&)
        {
            throw;
        }
    }
}
