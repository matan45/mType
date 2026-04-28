#include "InterfaceMethodSignatureParser.hpp"
#include "../TypeParser.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../utilities/ParameterParser.hpp"
#include "../class/GenericParameterParser.hpp"
#include "../../ast/nodes/functions/FunctionNode.hpp"
#include "../../ast/nodes/statements/BlockNode.hpp"
#include "../../ast/GenericType.hpp"
#include "../../errors/ParseException.hpp"
#include "../../errors/MissingSemicolonException.hpp"
#include "../../token/TokenType.hpp"

namespace parser
{
    using namespace ast::nodes::functions;
    using namespace ast::nodes::statements;
    using namespace token;
    using namespace errors;

    InterfaceMethodSignatureParser::InterfaceMethodSignatureParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }

    std::unique_ptr<ASTNode> InterfaceMethodSignatureParser::parse()
    {
        return parseMethodSignature();
    }

    bool InterfaceMethodSignatureParser::canParse(const TokenStream& stream) const
    {
        return stream.check(TokenType::FUNCTION);
    }

    std::unique_ptr<ASTNode> InterfaceMethodSignatureParser::parseMethodSignature()
    {
        // Check for 'static' keyword - not allowed in interfaces
        if (tokenStream.current().type == TokenType::STATIC)
        {
            throw ParseException("Static methods are not allowed in interfaces", tokenStream.current().location);
        }

        // Check for 'default' keyword
        bool isDefault = false;
        if (tokenStream.current().type == TokenType::DEFAULT)
        {
            isDefault = true;
            tokenStream.advance();
        }

        // Consume 'function' keyword
        if (tokenStream.current().type != TokenType::FUNCTION)
        {
            throw ParseException("Expected 'function' keyword", tokenStream.current().location);
        }
        tokenStream.advance();

        // Check for async keyword AFTER function keyword
        bool isAsync = false;
        if (tokenStream.current().type == TokenType::ASYNC)
        {
            isAsync = true;
            tokenStream.advance();
        }

        // Parse generic type parameters if present (method-level generics)
        std::vector<ast::GenericTypeParameter> methodGenericParameters;
        if (tokenStream.current().type == TokenType::LESS)
        {
            tokenStream.advance(); // consume '<'
            GenericParameterParser genericParser(tokenStream, context);
            methodGenericParameters = genericParser.parseGenericTypeParameters();

            if (tokenStream.current().type != TokenType::GREATER)
            {
                throw ParseException("Expected '>' after generic type parameters", tokenStream.current().location);
            }
            tokenStream.advance(); // consume '>'
        }

        // Parse function name
        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected method name", tokenStream.current().location);
        }

        std::string methodName = std::string(tokenStream.current().stringValue);
        tokenStream.advance();

        // Parse parameter list using ParameterParser
        auto parameters = ParameterParser::parseGenericParameterList(tokenStream, true);

        // Parse return type
        std::shared_ptr<GenericType> returnType;
        if (tokenStream.current().type == TokenType::COLON)
        {
            tokenStream.advance();

            // Parse return type using TypeParser (supports generic types and arrays)
            returnType = TypeParser::parseGenericType(tokenStream);
        }
        else
        {
            // Default to void if no return type specified
            returnType = std::make_shared<GenericType>(ValueType::VOID);
        }

        std::shared_ptr<ASTNode> body;

        if (isDefault)
        {
            // Default methods must have a body
            ParseContext::AsyncContextGuard asyncGuard(context.getContextState(), isAsync);
            body = context.parseStatement();
        }
        else
        {
            // Expect semicolon to end method signature
            if (tokenStream.current().type != TokenType::SEMICOLON)
            {
                // MYT-48 — typed exception so the LSP offers an "Insert ';'" fix.
                throw errors::MissingSemicolonException(tokenStream.current().location);
            }
            tokenStream.advance();

            // Create a dummy empty body for the method signature (interfaces don't have implementations)
            body = std::make_shared<BlockNode>(tokenStream.current().location);
        }

        // Create a method signature node using FunctionNode
        auto methodNode = std::make_unique<FunctionNode>(
            methodName, returnType, parameters, body
        );

        // Set generic type parameters for the method (e.g., <T>, <K, V>)
        methodNode->setGenericTypeParameters(methodGenericParameters);

        // Set async flag if this is an async method
        methodNode->setIsAsync(isAsync);

        // NEW: We need a way to mark this FunctionNode as default.
        // Wait, does FunctionNode have an isDefault field? We need to add it.
        // For now, let's assume we add it.
        methodNode->setDefault(isDefault);

        return std::move(methodNode);
    }
}
