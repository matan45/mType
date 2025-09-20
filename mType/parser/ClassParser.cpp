#include "ClassParser.hpp"
#include "TypeParser.hpp"
#include "ParserUtils.hpp"
#include "../services/ImportManager.hpp"
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
        tokenStream.expect(TokenType::CLASS);

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected class name", tokenStream.current().location);
        }

        std::string className = tokenStream.current().stringValue;

        // Validate class naming convention
        if (!ParserValidator::isValidClassName(className))
        {
            throw ParseException("Class name '" + className + "' must start with an uppercase letter",
                                 tokenStream.current().location);
        }

        tokenStream.advance();

        tokenStream.expect(TokenType::LBRACE);

        auto classNode = std::make_unique<ClassNode>(className);

        while (tokenStream.current().type != TokenType::RBRACE && tokenStream.current().type != TokenType::END)
        {
            TokenType currentToken = tokenStream.current().type;
            
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

        tokenStream.expect(TokenType::RBRACE);

        return std::move(classNode);
    }

    std::unique_ptr<ASTNode> ClassParser::parseConstructor()
    {
        tokenStream.expect(TokenType::CONSTRUCTOR);

        // Parse parameter list using centralized utility
        auto parameters = ParserUtils::parseParameterList(tokenStream, true);

        auto body = context.parseStatement();

        return std::make_unique<ConstructorNode>(std::move(parameters), std::move(body));
    }

    std::unique_ptr<ASTNode> ClassParser::parseMethod()
    {
        bool isStatic = false;

        // Handle static modifier
        if (tokenStream.current().type == TokenType::STATIC)
        {
            isStatic = true;
            tokenStream.advance();
        }

        // Handle function keyword (required for methods)
        if (tokenStream.current().type != TokenType::FUNCTION)
        {
            throw ParseException("Expected 'function' keyword", tokenStream.current().location);
        }
        tokenStream.advance();

        // Parse method name
        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected method name", tokenStream.current().location);
        }

        std::string methodName = tokenStream.current().stringValue;
        tokenStream.advance();

        // Parse parameter list using centralized utility
        auto parameters = ParserUtils::parseParameterList(tokenStream, true);

        // Parse return type after the parameters (mType syntax: function name(params): returnType)
        ValueType returnType = ValueType::VOID;
        if (tokenStream.current().type == TokenType::COLON)
        {
            tokenStream.advance();

            TokenType returnTokenType = tokenStream.current().type;
            if (returnTokenType == TokenType::INT)
            {
                returnType = ValueType::INT;
                tokenStream.advance();
            }
            else if (returnTokenType == TokenType::FLOAT)
            {
                returnType = ValueType::FLOAT;
                tokenStream.advance();
            }
            else if (returnTokenType == TokenType::BOOL)
            {
                returnType = ValueType::BOOL;
                tokenStream.advance();
            }
            else if (returnTokenType == TokenType::STRING_TYPE)
            {
                returnType = ValueType::STRING;
                tokenStream.advance();
            }
            else if (returnTokenType == TokenType::VOID)
            {
                returnType = ValueType::VOID;
                tokenStream.advance();
            }
            else if (returnTokenType == TokenType::IDENTIFIER)
            {
                std::string typeName = tokenStream.current().stringValue;
                tokenStream.advance();

                // Handle qualified names like geometry::Point
                while (tokenStream.current().type == TokenType::SCOPE)
                {
                    tokenStream.advance();
                    if (tokenStream.current().type != TokenType::IDENTIFIER)
                    {
                        throw ParseException("Expected identifier after '::'", tokenStream.current().location);
                    }
                    typeName += "::" + tokenStream.current().stringValue;
                    tokenStream.advance();
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
                throw ParseException("Expected return type after ':'", tokenStream.current().location);
            }
        }

        auto body = context.parseStatement();

        return std::make_unique<MethodNode>(methodName, returnType, std::move(parameters),
                                            std::move(body), isStatic);
    }

    std::unique_ptr<ASTNode> ClassParser::parseField()
    {
        bool isStatic = false;
        bool isFinal = false;

        if (tokenStream.current().type == TokenType::STATIC)
        {
            isStatic = true;
            tokenStream.advance();
        }

        if (tokenStream.current().type == TokenType::FINAL)
        {
            isFinal = true;
            tokenStream.advance();
        }

        // Check if this is actually a method (static/final function)
        if (tokenStream.current().type == TokenType::FUNCTION)
        {
            // Methods cannot be final - this is a syntax error
            if (isFinal)
            {
                throw ParseException("Methods cannot be final", tokenStream.current().location);
            }
            
            // This is a static method, parse it here since we already have the modifiers
            tokenStream.advance(); // consume 'function'
            
            // Parse method name
            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected method name", tokenStream.current().location);
            }
            
            std::string methodName = tokenStream.current().stringValue;
            tokenStream.advance();
            
            // Parse parameter list using centralized utility
            auto parameters = ParserUtils::parseParameterList(tokenStream, true);
            
            // Parse return type
            ValueType returnType = ValueType::VOID;
            if (tokenStream.current().type == TokenType::COLON)
            {
                tokenStream.advance();
                TokenType returnTokenType = tokenStream.current().type;
                if (returnTokenType == TokenType::INT)
                {
                    returnType = ValueType::INT;
                    tokenStream.advance();
                }
                else if (returnTokenType == TokenType::FLOAT)
                {
                    returnType = ValueType::FLOAT;
                    tokenStream.advance();
                }
                else if (returnTokenType == TokenType::BOOL)
                {
                    returnType = ValueType::BOOL;
                    tokenStream.advance();
                }
                else if (returnTokenType == TokenType::STRING_TYPE)
                {
                    returnType = ValueType::STRING;
                    tokenStream.advance();
                }
                else if (returnTokenType == TokenType::VOID)
                {
                    returnType = ValueType::VOID;
                    tokenStream.advance();
                }
                else if (returnTokenType == TokenType::IDENTIFIER)
                {
                    std::string typeName = tokenStream.current().stringValue;
                    tokenStream.advance();

                    // Handle qualified names like geometry::Point
                    while (tokenStream.current().type == TokenType::SCOPE)
                    {
                        tokenStream.advance();
                        if (tokenStream.current().type != TokenType::IDENTIFIER)
                        {
                            throw ParseException("Expected identifier after '::'", tokenStream.current().location);
                        }
                        typeName += "::" + tokenStream.current().stringValue;
                        tokenStream.advance();
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
                    throw ParseException("Expected return type after ':'", tokenStream.current().location);
                }
            }
            
            // Parse method body
            auto body = context.parseStatement();
            
            return std::make_unique<MethodNode>(methodName, returnType, std::move(parameters), std::move(body), isStatic);
        }

        // Parse the complete type information using TypeParser (supports collections and generics)
        parser::TypeInfo typeInfo = TypeParser::parseTypeInfo(tokenStream);
        ValueType fieldType = typeInfo.baseType;

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected field name", tokenStream.current().location);
        }

        std::string fieldName = tokenStream.current().stringValue;
        tokenStream.advance();

        std::unique_ptr<ASTNode> initialValue = nullptr;
        if (tokenStream.current().type == TokenType::ASSIGN)
        {
            tokenStream.advance();
            initialValue = context.parseExpression();
        }

        tokenStream.expect(TokenType::SEMICOLON);

        return std::make_unique<FieldNode>(fieldName, fieldType, std::move(initialValue),
                                           isStatic, isFinal);
    }

    std::unique_ptr<ASTNode> ClassParser::parseNewExpression()
    {
        tokenStream.expect(TokenType::NEW);

        // Check for valid type names - both identifiers and collection types
        TokenType currentType = tokenStream.current().type;
        if (currentType != TokenType::IDENTIFIER && 
            currentType != TokenType::ARRAY && 
            currentType != TokenType::MAP && 
            currentType != TokenType::SET && 
            currentType != TokenType::QUEUE && 
            currentType != TokenType::STACK)
        {
            throw ParseException("Expected class name after 'new'", tokenStream.current().location);
        }

        // Parse qualified class name (e.g., namespace::ClassName or collection type)
        std::vector<std::string> qualifiedParts;
        qualifiedParts.push_back(tokenStream.current().stringValue);
        tokenStream.advance();

        while (tokenStream.current().type == TokenType::SCOPE)
        {
            tokenStream.advance();
            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected identifier after '::'", tokenStream.current().location);
            }
            qualifiedParts.push_back(tokenStream.current().stringValue);
            tokenStream.advance();
        }

        // Validate only the final class name (not namespace parts)
        std::string finalClassName = qualifiedParts.back();
        if (!ParserValidator::isValidClassName(finalClassName))
        {
            throw ParseException("Class name '" + finalClassName + "' must start with an uppercase letter",
                                 tokenStream.current().location);
        }

        // Reconstruct full qualified name for the AST
        std::string className = qualifiedParts[0];
        for (size_t i = 1; i < qualifiedParts.size(); ++i)
        {
            className += "::" + qualifiedParts[i];
        }

        // Handle generic parameters using recursive parsing for nested types
        if (tokenStream.check(TokenType::LESS))
        {
            std::string genericsString = "<";
            tokenStream.advance(); // consume '<'
            
            // Use recursive approach to parse generic parameters
            genericsString += parseGenericParameters();
            
            tokenStream.expect(TokenType::GREATER); // consume '>'
            genericsString += ">";
            
            className += genericsString;
        }

        tokenStream.expect(TokenType::LPAREN);
        std::vector<std::unique_ptr<ASTNode>> arguments;
        if (!tokenStream.check(TokenType::RPAREN))
        {
            arguments.push_back(context.parseExpression());
            while (tokenStream.match(TokenType::COMMA))
            {
                arguments.push_back(context.parseExpression());
            }
        }
        tokenStream.expect(TokenType::RPAREN);

        return std::make_unique<NewNode>(className, std::move(arguments));
    }

    std::string ClassParser::parseGenericParameters()
    {
        std::string result;
        
        // Parse first parameter
        result += parseGenericParameter();
        
        // Parse additional parameters separated by commas
        while (tokenStream.check(TokenType::COMMA))
        {
            result += ", ";
            tokenStream.advance(); // consume ','
            result += parseGenericParameter();
        }
        
        return result;
    }
    
    std::string ClassParser::parseGenericParameter()
    {
        // Handle type name (could be identifier or built-in type)
        if (tokenStream.current().type == TokenType::IDENTIFIER ||
            tokenStream.current().type == TokenType::ARRAY ||
            tokenStream.current().type == TokenType::MAP ||
            tokenStream.current().type == TokenType::SET ||
            tokenStream.current().type == TokenType::QUEUE ||
            tokenStream.current().type == TokenType::STACK ||
            tokenStream.current().type == TokenType::INT ||
            tokenStream.current().type == TokenType::FLOAT ||
            tokenStream.current().type == TokenType::BOOL ||
            tokenStream.current().type == TokenType::STRING_TYPE)
        {
            std::string paramType = tokenStream.current().stringValue;
            tokenStream.advance();
            
            // Check for nested generics (e.g., Array<int> inside Array<Array<int>>)
            if (tokenStream.check(TokenType::LESS))
            {
                paramType += "<";
                tokenStream.advance(); // consume '<'
                paramType += parseGenericParameters(); // Recursive call
                tokenStream.expect(TokenType::GREATER); // consume '>'
                paramType += ">";
            }
            
            return paramType;
        }
        else
        {
            throw ParseException("Expected type parameter", tokenStream.current().location);
        }
    }

}
