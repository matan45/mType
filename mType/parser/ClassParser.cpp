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
            TokenType currentToken = parser.getCurrentToken().type;
            
            if (currentToken == TokenType::CONSTRUCTOR)
            {
                auto constructor = parseConstructor();
                if (constructor)
                {
                    classNode->addConstructor(std::move(constructor));
                }
            }
            else if (currentToken == TokenType::FUNCTION)
            {
                auto method = parseMethod();
                if (method)
                {
                    classNode->addMethod(std::move(method));
                }
            }
            else
            {
                // Default case - parse as field
                // This handles regular fields, static fields, final fields, and static methods
                auto field = parseField();
                if (field)
                {
                    // Check if the parsed field is actually a method (static function)
                    if (dynamic_cast<MethodNode*>(field.get()))
                    {
                        classNode->addMethod(std::move(field));
                    }
                    else
                    {
                        classNode->addField(std::move(field));
                    }
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
                parser.advanceToken();

                // Handle qualified names like geometry::Point
                while (parser.getCurrentToken().type == TokenType::SCOPE)
                {
                    parser.advanceToken();
                    if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
                    {
                        throw ParseException("Expected identifier after '::'", parser.getCurrentToken().location);
                    }
                    typeName += "::" + parser.getCurrentToken().stringValue;
                    parser.advanceToken();
                }

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
        bool isStatic = false;
        bool isFinal = false;

        // Handle static modifier
        if (parser.getCurrentToken().type == TokenType::STATIC)
        {
            isStatic = true;
            parser.advanceToken();
        }

        // Handle final modifier
        if (parser.getCurrentToken().type == TokenType::FINAL)
        {
            isFinal = true;
            parser.advanceToken();
        }

        // Handle function keyword (required for methods)
        if (parser.getCurrentToken().type != TokenType::FUNCTION)
        {
            throw ParseException("Expected 'function' keyword", parser.getCurrentToken().location);
        }
        parser.advanceToken();

        // Parse method name
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
                parser.advanceToken();

                // Handle qualified names like geometry::Point
                while (parser.getCurrentToken().type == TokenType::SCOPE)
                {
                    parser.advanceToken();
                    if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
                    {
                        throw ParseException("Expected identifier after '::'", parser.getCurrentToken().location);
                    }
                    typeName += "::" + parser.getCurrentToken().stringValue;
                    parser.advanceToken();
                }

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
                parser.advanceToken();

                // Handle qualified names like geometry::Point
                while (parser.getCurrentToken().type == TokenType::SCOPE)
                {
                    parser.advanceToken();
                    if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
                    {
                        throw ParseException("Expected identifier after '::'", parser.getCurrentToken().location);
                    }
                    typeName += "::" + parser.getCurrentToken().stringValue;
                    parser.advanceToken();
                }

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
            }
            else
            {
                throw ParseException("Expected return type after ':'", parser.getCurrentToken().location);
            }
        }

        auto body = parser.parseStatement();

        return std::make_unique<MethodNode>(methodName, returnType, std::move(parameters),
                                            std::move(body), isStatic);
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

        // Check if this is actually a method (static/final function)
        if (parser.getCurrentToken().type == TokenType::FUNCTION)
        {
            // Methods cannot be final - this is a syntax error
            if (isFinal)
            {
                throw ParseException("Methods cannot be final", parser.getCurrentToken().location);
            }
            
            // This is a static method, parse it here since we already have the modifiers
            parser.advanceToken(); // consume 'function'
            
            // Parse method name
            if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected method name", parser.getCurrentToken().location);
            }
            
            std::string methodName = parser.getCurrentToken().stringValue;
            parser.advanceToken();
            
            parser.expectToken(TokenType::LPAREN);
            
            // Parse parameters (simplified version of parseMethod's parameter parsing)
            std::vector<std::pair<std::string, ValueType>> parameters;
            while (parser.getCurrentToken().type != TokenType::RPAREN)
            {
                ValueType paramType = ValueType::VOID;
                TokenType currentType = parser.getCurrentToken().type;
                
                // Handle parameter types
                if (currentType == TokenType::INT) { paramType = ValueType::INT; parser.advanceToken(); }
                else if (currentType == TokenType::FLOAT) { paramType = ValueType::FLOAT; parser.advanceToken(); }
                else if (currentType == TokenType::BOOL) { paramType = ValueType::BOOL; parser.advanceToken(); }
                else if (currentType == TokenType::STRING_TYPE) { paramType = ValueType::STRING; parser.advanceToken(); }
                else if (currentType == TokenType::VOID) { paramType = ValueType::VOID; parser.advanceToken(); }
                else if (currentType == TokenType::IDENTIFIER) { paramType = ValueType::OBJECT; parser.advanceToken(); }
                else { throw ParseException("Expected parameter type", parser.getCurrentToken().location); }
                
                if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
                    throw ParseException("Expected parameter name", parser.getCurrentToken().location);
                
                std::string paramName = parser.getCurrentToken().stringValue;
                parameters.emplace_back(paramName, paramType);
                parser.advanceToken();
                
                if (parser.getCurrentToken().type == TokenType::COMMA)
                    parser.advanceToken();
                else if (parser.getCurrentToken().type != TokenType::RPAREN)
                    throw ParseException("Expected ',' or ')'", parser.getCurrentToken().location);
            }
            
            parser.expectToken(TokenType::RPAREN);
            
            // Parse return type
            ValueType returnType = ValueType::VOID;
            if (parser.getCurrentToken().type == TokenType::COLON)
            {
                parser.advanceToken();
                TokenType returnTokenType = parser.getCurrentToken().type;
                if (returnTokenType == TokenType::INT) returnType = ValueType::INT;
                else if (returnTokenType == TokenType::FLOAT) returnType = ValueType::FLOAT;
                else if (returnTokenType == TokenType::BOOL) returnType = ValueType::BOOL;
                else if (returnTokenType == TokenType::STRING_TYPE) returnType = ValueType::STRING;
                else if (returnTokenType == TokenType::VOID) returnType = ValueType::VOID;
                else if (returnTokenType == TokenType::IDENTIFIER) returnType = ValueType::OBJECT;
                else throw ParseException("Expected return type", parser.getCurrentToken().location);
                
                parser.advanceToken();
            }
            
            // Parse method body
            auto body = parser.parseStatement();
            
            return std::make_unique<MethodNode>(methodName, returnType, std::move(parameters), std::move(body), isStatic);
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
            parser.advanceToken();

            // Handle qualified names like geometry::Point
            while (parser.getCurrentToken().type == TokenType::SCOPE)
            {
                parser.advanceToken();
                if (parser.getCurrentToken().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected identifier after '::'", parser.getCurrentToken().location);
                }
                typeName += "::" + parser.getCurrentToken().stringValue;
                parser.advanceToken();
            }

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

}
