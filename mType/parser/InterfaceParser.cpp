#include "InterfaceParser.hpp"
#include "class/GenericParameterParser.hpp"
#include "TypeParser.hpp"
#include "utilities/ParserUtils.hpp"
#include "utilities/AccessModifierParser.hpp"
#include "utilities/VisibilityParser.hpp"
#include "../token/TokenType.hpp"
#include "../ast/nodes/classes/InterfaceNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace ast::nodes::classes;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::statements;
    using namespace token;
    using namespace errors;
    using namespace parser::utilities;

    InterfaceParser::InterfaceParser(TokenStream& stream, ParseContext& ctx)
        : tokenStream(stream), context(ctx)
    {
    }

    std::unique_ptr<InterfaceNode> InterfaceParser::parseInterface()
    {
        // Parse optional visibility modifier (public/private)
        // Default is PUBLIC if not specified
        VisibilityModifier visibility = VisibilityParser::parseVisibilityModifier(tokenStream);

        // Check for optional 'final' keyword
        bool isFinal = false;
        if (tokenStream.check(TokenType::FINAL))
        {
            isFinal = true;
            tokenStream.advance(); // consume 'final'
        }

        // Consume 'interface' token
        if (tokenStream.current().type != TokenType::INTERFACE)
        {
            throw ParseException("Expected 'interface' keyword", tokenStream.current().location);
        }
        tokenStream.advance();

        // Parse interface name
        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected interface name", tokenStream.current().location);
        }

        std::string interfaceName = tokenStream.current().stringValue.getString();
        auto location = tokenStream.current().location;

        // Validate interface name using ParserUtils
        ParserUtils::validateCapitalizedName(interfaceName, "Interface", location);

        tokenStream.advance();

        // Parse optional generic type parameters
        std::vector<GenericTypeParameter> genericParams;
        if (tokenStream.current().type == TokenType::LESS)
        {
            tokenStream.advance(); // consume '<'
            genericParams = parseGenericTypeParameters();
            tokenStream.expect(TokenType::GREATER); // consume '>'
        }

        // Create interface node
        auto interfaceNode = std::make_unique<InterfaceNode>(
            interfaceName, genericParams, tokenStream.current().location
        );

        // Parse optional extends clause
        if (tokenStream.current().type == TokenType::EXTENDS)
        {
            tokenStream.advance(); // consume 'extends'

            // Use ParserUtils to parse the interface list
            auto extendedInterfaces = ParserUtils::parseInterfaceList(tokenStream, "extends");
            for (const auto& interfaceName : extendedInterfaces)
            {
                interfaceNode->addExtendedInterface(interfaceName);
            }
        }

        // Parse interface body
        if (tokenStream.current().type != TokenType::LBRACE)
        {
            throw ParseException("Expected '{' to start interface body",
                                 tokenStream.current().location);
        }
        tokenStream.advance();

        // Parse method signatures
        while (tokenStream.current().type != TokenType::RBRACE && !tokenStream.isAtEnd())
        {
            // Check for access modifiers - validate they are PUBLIC or not present
            if (utilities::AccessModifierParser::isAccessModifier(tokenStream.current().type))
            {
                auto location = tokenStream.current().location;
                ast::AccessModifier modifier = utilities::AccessModifierParser::tokenTypeToAccessModifier(tokenStream.current().type);

                // Validate that interface methods can only be public
                utilities::AccessModifierParser::validateModifierForContext(modifier, true, location);

                tokenStream.advance(); // consume the access modifier
            }

            if (tokenStream.current().type == TokenType::FUNCTION)
            {
                auto methodSignature = parseMethodSignature();
                if (methodSignature)
                {
                    interfaceNode->addMethod(std::move(methodSignature));
                }
            }
            else
            {
                throw ParseException("Only function signatures allowed in interfaces",
                                     tokenStream.current().location);
            }
        }

        if (tokenStream.current().type != TokenType::RBRACE)
        {
            throw ParseException("Expected '}' to end interface body",
                                 tokenStream.current().location);
        }
        tokenStream.advance();

        interfaceNode->setFinal(isFinal);
        interfaceNode->setVisibility(visibility);
        return interfaceNode;
    }

    std::unique_ptr<ASTNode> InterfaceParser::parseMethodSignature()
    {
        // Check for 'static' keyword - not allowed in interfaces
        if (tokenStream.current().type == TokenType::STATIC)
        {
            throw ParseException("Static methods are not allowed in interfaces", tokenStream.current().location);
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

        // Parse function name
        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected method name", tokenStream.current().location);
        }

        std::string methodName = tokenStream.current().stringValue.getString();
        tokenStream.advance();

        // Parse parameter list using ParserUtils
        auto parameters = ParserUtils::parseGenericParameterList(tokenStream, true);

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

        // Expect semicolon to end method signature
        if (tokenStream.current().type != TokenType::SEMICOLON)
        {
            throw ParseException("Expected ';' after method signature", tokenStream.current().location);
        }
        tokenStream.advance();

        // Create a dummy empty body for the method signature (interfaces don't have implementations)
        auto dummyBody = std::make_shared<BlockNode>(tokenStream.current().location);

        // Create a method signature node using FunctionNode
        auto methodNode = std::make_unique<FunctionNode>(
            methodName, returnType, parameters, dummyBody
        );

        // Set async flag if this is an async method
        methodNode->setIsAsync(isAsync);

        return std::move(methodNode);
    }

    std::vector<GenericTypeParameter> InterfaceParser::parseGenericTypeParameters()
    {
        // Use GenericParameterParser to handle all the complexity
        GenericParameterParser genericParser(tokenStream, context);
        return genericParser.parseGenericTypeParameters();
    }
}
