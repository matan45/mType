#include "InterfaceParser.hpp"
#include "class/GenericParameterParser.hpp"
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

    std::unique_ptr<InterfaceNode> InterfaceParser::parseInterface()
    {
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

        // Validate interface name starts with capital letter
        if (interfaceName.empty() || !std::isupper(interfaceName[0]))
        {
            throw ParseException("Interface name '" + interfaceName + "' must start with a capital letter",
                                 tokenStream.current().location);
        }

        tokenStream.advance();

        // Parse optional generic type parameters
        std::vector<ast::GenericTypeParameter> genericParams;
        if (tokenStream.current().type == TokenType::LESS)
        {
            genericParams = parseGenericTypeParameters();
        }

        // Create interface node
        auto interfaceNode = std::make_unique<InterfaceNode>(
            interfaceName, genericParams, tokenStream.current().location
        );

        // Parse optional extends clause
        if (tokenStream.current().type == TokenType::EXTENDS)
        {
            tokenStream.advance(); // consume 'extends'

            // Parse parent interfaces (can be multiple)
            do
            {
                if (tokenStream.current().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected interface name after 'extends'",
                                         tokenStream.current().location);
                }

                std::string parentInterface = tokenStream.current().stringValue.getString();
                interfaceNode->addExtendedInterface(parentInterface);
                tokenStream.advance();

                // Handle generic parameters for parent interface
                if (tokenStream.current().type == TokenType::LESS)
                {
                    tokenStream.advance(); // consume '<'

                    // Use GenericParameterParser to properly parse the generic type parameters
                    GenericParameterParser genericParser(tokenStream, context);
                    auto parentGenericParams = genericParser.parseGenericTypeParameters();

                    tokenStream.expect(TokenType::GREATER); // consume '>'

                    // Store the generic parameters with the parent interface
                    // Note: This extends the parent interface name with generic info for now
                    // A more complete implementation would store these separately
                    if (!parentGenericParams.empty())
                    {
                        std::string genericSuffix = "<";
                        for (size_t i = 0; i < parentGenericParams.size(); ++i)
                        {
                            if (i > 0) genericSuffix += ", ";
                            genericSuffix += parentGenericParams[i].name;
                        }
                        genericSuffix += ">";
                        parentInterface += genericSuffix;
                        interfaceNode->addExtendedInterface(parentInterface);
                    }
                }

                // Check for comma (multiple inheritance)
                if (tokenStream.current().type == TokenType::COMMA)
                {
                    tokenStream.advance();
                }
                else
                {
                    break;
                }
            }
            while (true);
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

        return interfaceNode;
    }

    std::unique_ptr<ASTNode> InterfaceParser::parseMethodSignature()
    {
        // Consume 'function' keyword
        if (tokenStream.current().type != TokenType::FUNCTION)
        {
            throw ParseException("Expected 'function' keyword", tokenStream.current().location);
        }
        tokenStream.advance();

        // Parse function name
        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected method name", tokenStream.current().location);
        }

        std::string methodName = tokenStream.current().stringValue.getString();
        tokenStream.advance();

        // Parse parameters
        if (tokenStream.current().type != TokenType::LPAREN)
        {
            throw ParseException("Expected '(' after method name", tokenStream.current().location);
        }
        tokenStream.advance();

        std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>> parameters;

        // Parse parameter list
        while (tokenStream.current().type != TokenType::RPAREN && !tokenStream.isAtEnd())
        {
            // Parse parameter type (with support for generic types)
            std::string paramType;
            if (tokenStream.current().type == TokenType::IDENTIFIER)
            {
                paramType = tokenStream.current().stringValue.getString();
                tokenStream.advance();

                // Handle generic type parameters (e.g., Repository<T, ID>)
                if (tokenStream.current().type == TokenType::LESS)
                {
                    paramType += "<";
                    int depth = 1;
                    tokenStream.advance();

                    while (depth > 0 && !tokenStream.isAtEnd())
                    {
                        if (tokenStream.current().type == TokenType::LESS)
                        {
                            depth++;
                            paramType += "<";
                        }
                        else if (tokenStream.current().type == TokenType::GREATER)
                        {
                            depth--;
                            paramType += ">";
                        }
                        else if (tokenStream.current().type == TokenType::IDENTIFIER)
                        {
                            paramType += tokenStream.current().stringValue.getString();
                        }
                        else if (tokenStream.current().type == TokenType::COMMA)
                        {
                            paramType += ", ";
                        }
                        else
                        {
                            // Add other tokens as-is (for complex generic expressions)
                            paramType += tokenStream.current().stringValue.getString();
                        }
                        tokenStream.advance();
                    }
                }
            }
            else if (tokenStream.current().type == TokenType::INT)
            {
                paramType = "int";
                tokenStream.advance();
            }
            else if (tokenStream.current().type == TokenType::FLOAT)
            {
                paramType = "float";
                tokenStream.advance();
            }
            else if (tokenStream.current().type == TokenType::BOOL)
            {
                paramType = "bool";
                tokenStream.advance();
            }
            else if (tokenStream.current().type == TokenType::STRING_TYPE)
            {
                paramType = "string";
                tokenStream.advance();
            }
            else
            {
                throw ParseException("Expected parameter type", tokenStream.current().location);
            }

            // Parse parameter name
            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected parameter name", tokenStream.current().location);
            }

            std::string paramName = tokenStream.current().stringValue.getString();
            tokenStream.advance();

            // Create GenericType for parameter
            auto genericType = std::make_shared<ast::GenericType>(paramType);
            parameters.emplace_back(paramName, genericType);

            // Check for comma
            if (tokenStream.current().type == TokenType::COMMA)
            {
                tokenStream.advance();
            }
            else if (tokenStream.current().type != TokenType::RPAREN)
            {
                throw ParseException("Expected ',' or ')' in parameter list",
                                     tokenStream.current().location);
            }
        }

        if (tokenStream.current().type != TokenType::RPAREN)
        {
            throw ParseException("Expected ')' after parameter list", tokenStream.current().location);
        }
        tokenStream.advance();

        // Parse return type
        std::shared_ptr<ast::GenericType> returnType;
        if (tokenStream.current().type == TokenType::COLON)
        {
            tokenStream.advance();

            std::string returnTypeName;
            if (tokenStream.current().type == TokenType::IDENTIFIER)
            {
                returnTypeName = tokenStream.current().stringValue.getString();
                tokenStream.advance();

                // Handle generic return types (e.g., CacheStore<K, V, M>)
                if (tokenStream.current().type == TokenType::LESS)
                {
                    returnTypeName += "<";
                    int depth = 1;
                    tokenStream.advance();

                    while (depth > 0 && !tokenStream.isAtEnd())
                    {
                        if (tokenStream.current().type == TokenType::LESS)
                        {
                            depth++;
                            returnTypeName += "<";
                        }
                        else if (tokenStream.current().type == TokenType::GREATER)
                        {
                            depth--;
                            returnTypeName += ">";
                        }
                        else if (tokenStream.current().type == TokenType::IDENTIFIER)
                        {
                            returnTypeName += tokenStream.current().stringValue.getString();
                        }
                        else if (tokenStream.current().type == TokenType::COMMA)
                        {
                            returnTypeName += ", ";
                        }
                        else
                        {
                            // Add other tokens as-is (for complex generic expressions)
                            returnTypeName += tokenStream.current().stringValue.getString();
                        }
                        tokenStream.advance();
                    }
                }
            }
            else if (tokenStream.current().type == TokenType::VOID)
            {
                returnTypeName = "void";
                tokenStream.advance();
            }
            else if (tokenStream.current().type == TokenType::INT)
            {
                returnTypeName = "int";
                tokenStream.advance();
            }
            else if (tokenStream.current().type == TokenType::FLOAT)
            {
                returnTypeName = "float";
                tokenStream.advance();
            }
            else if (tokenStream.current().type == TokenType::BOOL)
            {
                returnTypeName = "bool";
                tokenStream.advance();
            }
            else if (tokenStream.current().type == TokenType::STRING_TYPE)
            {
                returnTypeName = "string";
                tokenStream.advance();
            }
            else
            {
                throw ParseException("Expected return type after ':'", tokenStream.current().location);
            }

            returnType = std::make_shared<ast::GenericType>(returnTypeName);
        }
        else
        {
            // Default to void if no return type specified
            returnType = std::make_shared<ast::GenericType>("void");
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

        return std::move(methodNode);
    }

    std::vector<ast::GenericTypeParameter> InterfaceParser::parseGenericTypeParameters()
    {
        std::vector<ast::GenericTypeParameter> genericParams;

        if (tokenStream.current().type != TokenType::LESS)
        {
            return genericParams;
        }
        tokenStream.advance(); // consume '<'

        do
        {
            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected generic type parameter name",
                                     tokenStream.current().location);
            }

            std::string paramName = tokenStream.current().stringValue.getString();
            tokenStream.advance();

            // Create generic type parameter
            ast::GenericTypeParameter param(paramName, tokenStream.current().location);

            // Parse optional constraints (extends/implements SomeInterface)
            if (tokenStream.current().type == TokenType::EXTENDS || tokenStream.current().type == TokenType::IMPLEMENTS)
            {
                tokenStream.advance();

                if (tokenStream.current().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected interface name after constraint keyword",
                                         tokenStream.current().location);
                }

                std::string constraintName = tokenStream.current().stringValue.getString();
                tokenStream.advance();

                // Handle generic parameters in constraints (e.g., Comparable<T>)
                if (tokenStream.current().type == TokenType::LESS)
                {
                    constraintName += "<";
                    int depth = 1;
                    tokenStream.advance();

                    while (depth > 0 && !tokenStream.isAtEnd())
                    {
                        if (tokenStream.current().type == TokenType::LESS)
                        {
                            depth++;
                            constraintName += "<";
                        }
                        else if (tokenStream.current().type == TokenType::GREATER)
                        {
                            depth--;
                            constraintName += ">";
                        }
                        else if (tokenStream.current().type == TokenType::IDENTIFIER)
                        {
                            constraintName += tokenStream.current().stringValue.getString();
                        }
                        else if (tokenStream.current().type == TokenType::COMMA)
                        {
                            constraintName += ",";
                        }
                        tokenStream.advance();
                    }
                }

                param.constraints.push_back(constraintName);
            }

            genericParams.push_back(param);

            if (tokenStream.current().type == TokenType::COMMA)
            {
                tokenStream.advance();
            }
            else
            {
                break;
            }
        }
        while (true);

        if (tokenStream.current().type != TokenType::GREATER)
        {
            throw ParseException("Expected '>' to close generic type parameters",
                                 tokenStream.current().location);
        }
        tokenStream.advance();

        return genericParams;
    }
}
