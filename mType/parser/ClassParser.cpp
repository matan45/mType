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

        // NEW: Parse generic type parameters if present (e.g., class Box<T>)
        std::vector<ast::GenericTypeParameter> genericParameters;
        if (tokenStream.check(TokenType::LESS))
        {
            tokenStream.advance(); // consume '<'
            genericParameters = parseGenericTypeParameters();
            tokenStream.expect(TokenType::GREATER); // consume '>'
        }

        tokenStream.expect(TokenType::LBRACE);

        // Create class node with generic parameters
        auto classNode = std::make_unique<ClassNode>(className, genericParameters);

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

        // Parse generic type parameters for generic methods (only for non-static methods)
        std::vector<ast::GenericTypeParameter> methodGenericParameters;
        if (!isStatic && tokenStream.check(TokenType::LESS))
        {
            tokenStream.advance(); // consume '<'
            methodGenericParameters = parseGenericTypeParameters();
            tokenStream.expect(TokenType::GREATER); // consume '>'
        }

        // Parse method name
        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected method name", tokenStream.current().location);
        }

        std::string methodName = tokenStream.current().stringValue;
        tokenStream.advance();

        // Parse parameter list using generic-aware utility
        auto parameters = ParserUtils::parseGenericParameterList(tokenStream, true);

        // Parse return type after the parameters using new generic type system
        std::shared_ptr<ast::GenericType> returnType = std::make_shared<ast::GenericType>(ValueType::VOID);
        if (tokenStream.current().type == TokenType::COLON)
        {
            tokenStream.advance();
            returnType = TypeParser::parseGenericType(tokenStream);
        }

        auto body = context.parseStatement();

        // Create MethodNode with generic support - always use the new constructor
        return std::make_unique<MethodNode>(methodName, returnType, std::move(parameters),
                                            std::move(body), isStatic, methodGenericParameters);
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

            // Static methods do not support generic type parameters
            std::vector<ast::GenericTypeParameter> methodGenericParameters;

            // Parse method name
            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected method name", tokenStream.current().location);
            }
            
            std::string methodName = tokenStream.current().stringValue;
            tokenStream.advance();
            
            // Parse parameter list using generic-aware utility
            auto parameters = ParserUtils::parseGenericParameterList(tokenStream, true);

            // Parse return type using generic type system
            std::shared_ptr<ast::GenericType> returnType = std::make_shared<ast::GenericType>(ValueType::VOID);
            if (tokenStream.current().type == TokenType::COLON)
            {
                tokenStream.advance();
                returnType = TypeParser::parseGenericType(tokenStream);
            }
            
            // Parse method body
            auto body = context.parseStatement();
            
            // Create MethodNode with generic support - always use the new constructor
            return std::make_unique<MethodNode>(methodName, returnType, std::move(parameters),
                                                std::move(body), isStatic, methodGenericParameters);
        }

        // Parse the complete type information using TypeParser (supports collections and generics)
        parser::TypeInfo typeInfo = TypeParser::parseTypeInfo(tokenStream);
        auto fieldGenericType = convertTypeInfoToGenericType(typeInfo);

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

        return std::make_unique<FieldNode>(fieldName, fieldGenericType, std::move(initialValue),
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

    // NEW: Parse generic type parameter declarations (e.g., <T, U, K extends Comparable>)
    std::vector<ast::GenericTypeParameter> ClassParser::parseGenericTypeParameters()
    {
        std::vector<ast::GenericTypeParameter> parameters;

        // Parse first parameter
        parameters.push_back(parseGenericTypeParameter());

        // Parse additional parameters separated by commas
        while (tokenStream.check(TokenType::COMMA))
        {
            tokenStream.advance(); // consume ','
            parameters.push_back(parseGenericTypeParameter());
        }

        return parameters;
    }

    // NEW: Parse a single generic type parameter declaration (e.g., T, U, K extends Comparable)
    ast::GenericTypeParameter ClassParser::parseGenericTypeParameter()
    {
        // Expect an identifier for the type parameter name
        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected generic type parameter name", tokenStream.current().location);
        }

        std::string paramName = tokenStream.current().stringValue;
        auto location = tokenStream.current().location;
        tokenStream.advance();

        // For now, we don't support constraints (extends/implements)
        // This can be added later when we implement bounded generics
        std::vector<std::string> constraints;

        // TODO: Future enhancement - parse constraints
        // if (tokenStream.check("extends")) {
        //     tokenStream.advance();
        //     constraints.push_back(parseTypeName());
        // }

        return ast::GenericTypeParameter(paramName, constraints, location);
    }

    // NEW: Convert TypeInfo to GenericType for field and method parsing
    std::shared_ptr<ast::GenericType> ClassParser::convertTypeInfoToGenericType(const TypeInfo& typeInfo)
    {
        // Handle basic types
        if (typeInfo.baseType != ValueType::OBJECT)
        {
            // For collection types, handle their generic arguments
            if (typeInfo.hasNestedElementType())
            {
                // Single-element collections (Array, Set, Queue, Stack)
                auto elementType = convertTypeInfoToGenericType(*typeInfo.getElementTypeInfo());
                return std::make_shared<ast::GenericType>(typeInfo.baseType,
                    std::vector<std::shared_ptr<ast::GenericType>>{elementType});
            }
            else if (typeInfo.hasNestedKeyType() && typeInfo.hasNestedValueType())
            {
                // Map type
                auto keyType = convertTypeInfoToGenericType(*typeInfo.getKeyTypeInfo());
                auto valueType = convertTypeInfoToGenericType(*typeInfo.getValueTypeInfo());
                return std::make_shared<ast::GenericType>(typeInfo.baseType,
                    std::vector<std::shared_ptr<ast::GenericType>>{keyType, valueType});
            }
            else
            {
                // Simple built-in types (int, float, bool, string, void)
                return std::make_shared<ast::GenericType>(typeInfo.baseType);
            }
        }
        else
        {
            // Handle object types
            if (!typeInfo.className.empty())
            {
                // For now, treat as regular object type
                // Later we can enhance this to detect generic type parameters
                return std::make_shared<ast::GenericType>(typeInfo.className);
            }
            else
            {
                // Generic object type
                return std::make_shared<ast::GenericType>(ValueType::OBJECT);
            }
        }
    }

}
