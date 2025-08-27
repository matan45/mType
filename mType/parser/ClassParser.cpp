#include "ClassParser.hpp"
#include "Parser.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/classes/ConstructorNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../ast/nodes/classes/FieldNode.hpp"
#include "../ast/nodes/classes/NewNode.hpp"
#include "../parser/ParserValidator.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace ast::nodes::classes;
    using namespace token;
    using namespace value;
    using namespace errors;


    std::unique_ptr<ASTNode> ClassParser::parseClass()
    {
        parser.expectToken(TokenType::CLASS);

        if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected class name", parser.getCurrentToken().location);
        }

        std::string className = parser.getCurrentToken().stringValue;

        // Validate class naming convention
        if (!ParserValidator::isValidClassName(className))
        {
            throw ParseException("Class name '" + className + "' must start with an uppercase letter",
                                 parser.getCurrentToken().location);
        }

        parser.advanceToken();

        parser.expectToken(TokenType::LBRACE);

        auto classNode = std::make_unique<ClassNode>(className);

        while (parser.getCurrentToken().type != TokenType::RBRACE && parser.getCurrentToken().type != TokenType::END)
        {
            if (isMethodOrConstructor())
            {
                if (parser.getCurrentToken().type == TokenType::CONSTRUCTOR)
                {
                    auto constructor = parseConstructor();
                    if (constructor)
                    {
                        classNode->addConstructor(std::move(constructor));
                    }
                }
                else
                {
                    auto method = parseMethod();
                    if (method)
                    {
                        classNode->addMethod(std::move(method));
                    }
                }
            }
            else
            {
                auto field = parseField();
                if (field)
                {
                    classNode->addField(std::move(field));
                }
            }
        }

        parser.expectToken(TokenType::RBRACE);

        return std::move(classNode);
    }

    std::unique_ptr<ASTNode> ClassParser::parseConstructor()
    {
        parser.expectToken(TokenType::CONSTRUCTOR);

        parser.expectToken(TokenType::LPAREN);

        std::vector<std::pair<std::string, ValueType>> parameters;
        while (parser.getCurrentToken().type != TokenType::RPAREN)
        {
            ValueType paramType = ValueType::VOID;
            TokenType currentType = parser.getCurrentToken().type;

            // Handle both dedicated type tokens and identifier-based types
            if (currentType == TokenType::INT)
            {
                paramType = ValueType::INT;
                parser.advanceToken();
            }
            else if (currentType == TokenType::FLOAT)
            {
                paramType = ValueType::FLOAT;
                parser.advanceToken();
            }
            else if (currentType == TokenType::BOOL)
            {
                paramType = ValueType::BOOL;
                parser.advanceToken();
            }
            else if (currentType == TokenType::STRING_TYPE)
            {
                paramType = ValueType::STRING;
                parser.advanceToken();
            }
            else if (currentType == TokenType::VOID)
            {
                paramType = ValueType::VOID;
                parser.advanceToken();
            }
            else if (currentType == TokenType::IDENTIFIER)
            {
                std::string typeName = parser.getCurrentToken().stringValue;
                if (typeName == "int") paramType = ValueType::INT;
                else if (typeName == "float") paramType = ValueType::FLOAT;
                else if (typeName == "string") paramType = ValueType::STRING;
                else if (typeName == "bool") paramType = ValueType::BOOL;
                else if (typeName == "void") paramType = ValueType::VOID;
                else
                {
                    // Treat unknown identifier types as custom class types (OBJECT)
                    paramType = ValueType::OBJECT;
                }
                parser.advanceToken();
            }
            else
            {
                throw ParseException("Expected parameter type", parser.getCurrentToken().location);
            }

            if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected parameter name", parser.getCurrentToken().location);
            }

            std::string paramName = parser.getCurrentToken().stringValue;
            parameters.push_back({paramName, paramType});
            parser.advanceToken();

            if (parser.getCurrentToken().type == TokenType::COMMA)
            {
                parser.advanceToken();
            }
        }

        parser.expectToken(TokenType::RPAREN);

        auto body = parser.parseStatement();

        return std::make_unique<ConstructorNode>(std::move(parameters), std::move(body));
    }

    std::unique_ptr<ASTNode> ClassParser::parseMethod()
    {
        // Handle optional function keyword for methods
        if (parser.getCurrentToken().type == TokenType::FUNCTION)
        {
            parser.advanceToken();
        }

        // Parse method name first
        if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected method name", parser.getCurrentToken().location);
        }

        std::string methodName = parser.getCurrentToken().stringValue;
        parser.advanceToken();

        parser.expectToken(TokenType::LPAREN);

        std::vector<std::pair<std::string, ValueType>> parameters;
        while (parser.getCurrentToken().type != TokenType::RPAREN)
        {
            ValueType paramType = ValueType::VOID;
            TokenType currentType = parser.getCurrentToken().type;

            // Handle both dedicated type tokens and identifier-based types
            if (currentType == TokenType::INT)
            {
                paramType = ValueType::INT;
                parser.advanceToken();
            }
            else if (currentType == TokenType::FLOAT)
            {
                paramType = ValueType::FLOAT;
                parser.advanceToken();
            }
            else if (currentType == TokenType::BOOL)
            {
                paramType = ValueType::BOOL;
                parser.advanceToken();
            }
            else if (currentType == TokenType::STRING_TYPE)
            {
                paramType = ValueType::STRING;
                parser.advanceToken();
            }
            else if (currentType == TokenType::VOID)
            {
                paramType = ValueType::VOID;
                parser.advanceToken();
            }
            else if (currentType == TokenType::IDENTIFIER)
            {
                std::string typeName = parser.getCurrentToken().stringValue;
                if (typeName == "int") paramType = ValueType::INT;
                else if (typeName == "float") paramType = ValueType::FLOAT;
                else if (typeName == "string") paramType = ValueType::STRING;
                else if (typeName == "bool") paramType = ValueType::BOOL;
                else if (typeName == "void") paramType = ValueType::VOID;
                else
                {
                    // Treat unknown identifier types as custom class types (OBJECT)
                    paramType = ValueType::OBJECT;
                }
                parser.advanceToken();
            }
            else
            {
                throw ParseException("Expected parameter type", parser.getCurrentToken().location);
            }

            if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected parameter name", parser.getCurrentToken().location);
            }

            std::string paramName = parser.getCurrentToken().stringValue;
            parameters.push_back({paramName, paramType});
            parser.advanceToken();

            if (parser.getCurrentToken().type == TokenType::COMMA)
            {
                parser.advanceToken();
            }
        }

        parser.expectToken(TokenType::RPAREN);

        // Parse return type after the parameters (mType syntax: function name(params): returnType)
        ValueType returnType = ValueType::VOID;
        if (parser.getCurrentToken().type == TokenType::COLON)
        {
            parser.advanceToken();

            TokenType returnTokenType = parser.getCurrentToken().type;
            if (returnTokenType == TokenType::INT)
            {
                returnType = ValueType::INT;
                parser.advanceToken();
            }
            else if (returnTokenType == TokenType::FLOAT)
            {
                returnType = ValueType::FLOAT;
                parser.advanceToken();
            }
            else if (returnTokenType == TokenType::BOOL)
            {
                returnType = ValueType::BOOL;
                parser.advanceToken();
            }
            else if (returnTokenType == TokenType::STRING_TYPE)
            {
                returnType = ValueType::STRING;
                parser.advanceToken();
            }
            else if (returnTokenType == TokenType::VOID)
            {
                returnType = ValueType::VOID;
                parser.advanceToken();
            }
            else if (returnTokenType == TokenType::IDENTIFIER)
            {
                std::string typeName = parser.getCurrentToken().stringValue;
                if (typeName == "int") returnType = ValueType::INT;
                else if (typeName == "float") returnType = ValueType::FLOAT;
                else if (typeName == "string") returnType = ValueType::STRING;
                else if (typeName == "bool") returnType = ValueType::BOOL;
                else if (typeName == "void") returnType = ValueType::VOID;
                else
                {
                    // Treat unknown identifier types as custom class types (OBJECT)
                    returnType = ValueType::OBJECT;
                }
                parser.advanceToken();
            }
            else
            {
                throw ParseException("Expected return type after ':'", parser.getCurrentToken().location);
            }
        }

        auto body = parser.parseStatement();

        return std::make_unique<MethodNode>(methodName, returnType, std::move(parameters),
                                            std::move(body), false);
    }

    std::unique_ptr<ASTNode> ClassParser::parseField()
    {
        bool isStatic = false;
        bool isFinal = false;

        if (parser.getCurrentToken().type == TokenType::STATIC)
        {
            isStatic = true;
            parser.advanceToken();
        }

        if (parser.getCurrentToken().type == TokenType::FINAL)
        {
            isFinal = true;
            parser.advanceToken();
        }

        ValueType fieldType = ValueType::VOID;

        // Handle both dedicated type tokens and identifier-based types
        TokenType currentType = parser.getCurrentToken().type;

        if (currentType == TokenType::INT)
        {
            fieldType = ValueType::INT;
            parser.advanceToken();
        }
        else if (currentType == TokenType::FLOAT)
        {
            fieldType = ValueType::FLOAT;
            parser.advanceToken();
        }
        else if (currentType == TokenType::BOOL)
        {
            fieldType = ValueType::BOOL;
            parser.advanceToken();
        }
        else if (currentType == TokenType::STRING_TYPE)
        {
            fieldType = ValueType::STRING;
            parser.advanceToken();
        }
        else if (currentType == TokenType::VOID)
        {
            fieldType = ValueType::VOID;
            parser.advanceToken();
        }
        else if (currentType == TokenType::IDENTIFIER)
        {
            // Handle string as identifier for backwards compatibility
            std::string typeName = parser.getCurrentToken().stringValue;
            if (typeName == "int") fieldType = ValueType::INT;
            else if (typeName == "float") fieldType = ValueType::FLOAT;
            else if (typeName == "string") fieldType = ValueType::STRING;
            else if (typeName == "bool") fieldType = ValueType::BOOL;
            else if (typeName == "void") fieldType = ValueType::VOID;
            else
            {
                // Treat unknown identifier types as custom class types (OBJECT)
                fieldType = ValueType::OBJECT;
            }

            parser.advanceToken();
        }

        if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected field name", parser.getCurrentToken().location);
        }

        std::string fieldName = parser.getCurrentToken().stringValue;
        parser.advanceToken();

        std::unique_ptr<ASTNode> initialValue = nullptr;
        if (parser.getCurrentToken().type == TokenType::ASSIGN)
        {
            parser.advanceToken();
            initialValue = parser.parseExpression();
        }

        parser.expectToken(TokenType::SEMICOLON);

        return std::make_unique<FieldNode>(fieldName, fieldType, std::move(initialValue),
                                           isStatic, isFinal);
    }

    std::unique_ptr<ASTNode> ClassParser::parseNewExpression()
    {
        parser.expectToken(TokenType::NEW);

        if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected class name after 'new'", parser.getCurrentToken().location);
        }

        // Parse qualified class name (e.g., namespace::ClassName)
        std::vector<std::string> qualifiedParts;
        qualifiedParts.push_back(parser.getCurrentToken().stringValue);
        parser.advanceToken();

        while (parser.getCurrentToken().type == TokenType::SCOPE)
        {
            parser.advanceToken();
            if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected identifier after '::'", parser.getCurrentToken().location);
            }
            qualifiedParts.push_back(parser.getCurrentToken().stringValue);
            parser.advanceToken();
        }

        // Validate only the final class name (not namespace parts)
        std::string finalClassName = qualifiedParts.back();
        if (!ParserValidator::isValidClassName(finalClassName))
        {
            throw ParseException("Class name '" + finalClassName + "' must start with an uppercase letter",
                                 parser.getCurrentToken().location);
        }

        // Reconstruct full qualified name for the AST
        std::string className = qualifiedParts[0];
        for (size_t i = 1; i < qualifiedParts.size(); ++i)
        {
            className += "::" + qualifiedParts[i];
        }

        parser.expectToken(TokenType::LPAREN);
        auto arguments = parser.getExpressionParser()->parseArguments();
        parser.expectToken(TokenType::RPAREN);

        return std::make_unique<NewNode>(className, std::move(arguments));
    }

    bool ClassParser::isMethodOrConstructor()
    {
        if (parser.getCurrentToken().type == TokenType::CONSTRUCTOR)
        {
            return true;
        }

        if (parser.getCurrentToken().type == TokenType::FUNCTION)
        {
            return true;
        }

        // Check if current token could start a method or field
        TokenType currentType = parser.getCurrentToken().type;

        // Handle static/final modifiers
        int tokenOffset = 0;
        if (currentType == TokenType::STATIC || currentType == TokenType::FINAL)
        {
            tokenOffset++;
            // For now, we'll use a simple heuristic since we can only peek one token ahead
            // If we see static/final followed by a type token, assume it could be either
            Token nextToken = parser.peekNextToken();
            if (nextToken.type == TokenType::INT || nextToken.type == TokenType::FLOAT ||
                nextToken.type == TokenType::BOOL || nextToken.type == TokenType::STRING_TYPE ||
                nextToken.type == TokenType::VOID)
            {
                // This is a static/final field or method - we need more lookahead to determine
                // For now, treat as field (conservative approach)
                return false;
            }
        }

        // Check if current token is a type that could start a method or field
        bool couldBeTypeDeclaration = (currentType == TokenType::INT ||
            currentType == TokenType::FLOAT ||
            currentType == TokenType::BOOL ||
            currentType == TokenType::STRING_TYPE ||
            currentType == TokenType::VOID ||
            currentType == TokenType::IDENTIFIER); // For custom types

        if (!couldBeTypeDeclaration)
        {
            return false;
        }

        // Look ahead to distinguish between method and field
        Token nextToken = parser.peekNextToken();

        // If next token is identifier, this could be either method or field
        // We need to implement a simple heuristic:
        // - If it's void type, it's likely a method
        // - If it's followed by an identifier and we can't see further, assume field for safety
        if (currentType == TokenType::VOID)
        {
            return true; // void can only be a method return type
        }

        // For other types, we need better heuristics
        // If next token is an identifier, check if the token after that gives us a clue
        if (nextToken.type == TokenType::IDENTIFIER)
        {
            // This could be either "type fieldName;" or "type methodName("
            // We can't easily peek two tokens ahead, so for now:
            // - If we see "function", it's definitely a method 
            // - Otherwise, assume field for safety
            return false;
        }

        // If we can't determine conclusively, assume field for safety
        return false;
    }
}
