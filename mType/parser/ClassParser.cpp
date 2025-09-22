#include "ClassParser.hpp"
#include "TypeParser.hpp"
#include "ParserUtils.hpp"
#include "../services/ImportManager.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/classes/ConstructorNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../ast/nodes/classes/FieldNode.hpp"
#include "../ast/nodes/classes/NewNode.hpp"
#include "../ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../parser/ParserValidator.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace ast::nodes::classes;
    using namespace ast::nodes::expressions;
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

        // Check for valid type names - identifiers, primitive types, and collection types
        TokenType currentType = tokenStream.current().type;
        if (currentType != TokenType::IDENTIFIER &&
            currentType != TokenType::INT &&
            currentType != TokenType::FLOAT &&
            currentType != TokenType::BOOL &&
            currentType != TokenType::STRING_TYPE &&
            true)
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

        // Check if this is array creation (new Type[size]) or class instantiation (new Type())
        if (tokenStream.check(TokenType::LBRACKET))
        {
            // Array creation: new T[size] or new int[size] or new int[size1][size2]
            std::vector<std::unique_ptr<ASTNode>> sizeExpressions;

            // Parse all dimensions
            while (tokenStream.check(TokenType::LBRACKET))
            {
                tokenStream.advance(); // consume '['

                // Parse the size expression for this dimension
                auto sizeExpression = context.parseExpression();
                sizeExpressions.push_back(std::move(sizeExpression));

                tokenStream.expect(TokenType::RBRACKET); // consume ']'
            }

            // Create TypeInfo from the type name
            TypeInfo elementTypeInfo;

            // Check if it's a primitive type
            if (className == "int") {
                elementTypeInfo = TypeInfo(ValueType::INT);
            } else if (className == "float") {
                elementTypeInfo = TypeInfo(ValueType::FLOAT);
            } else if (className == "bool") {
                elementTypeInfo = TypeInfo(ValueType::BOOL);
            } else if (className == "string") {
                elementTypeInfo = TypeInfo(ValueType::STRING);
            } else {
                // Generic type parameter or custom class - treat as OBJECT
                elementTypeInfo = TypeInfo(ValueType::OBJECT, className);
            }

            return std::make_unique<ArrayCreationNode>(elementTypeInfo, std::move(sizeExpressions));
        }
        else
        {
            // Class instantiation: new Type()
            // Validate class name for actual class instantiation (not array creation)
            std::string finalClassName = qualifiedParts.back();
            if (!ParserValidator::isValidClassName(finalClassName))
            {
                throw ParseException("Class name '" + finalClassName + "' must start with an uppercase letter",
                                     tokenStream.current().location);
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

    // Helper method to create TypeInfo from class name string
    TypeInfo ClassParser::createTypeInfoFromClassName(const std::string& className)
    {
        // Handle basic types
        if (className == "int") return TypeInfo(ValueType::INT);
        if (className == "float") return TypeInfo(ValueType::FLOAT);
        if (className == "bool") return TypeInfo(ValueType::BOOL);
        if (className == "string") return TypeInfo(ValueType::STRING);
        if (className == "void") return TypeInfo(ValueType::VOID);


        // For complex generic types like "Array<int>", extract and parse
        size_t anglePos = className.find('<');
        if (anglePos != std::string::npos)
        {
            std::string baseType = className.substr(0, anglePos);
            std::string genericPart = className.substr(anglePos + 1, className.length() - anglePos - 2); // Remove < and >

            // Custom generic class - treat as object for now
            return TypeInfo(ValueType::OBJECT, className);
        }

        // Default: treat as custom class (OBJECT type)
        return TypeInfo(ValueType::OBJECT, className);
    }

    // NEW: Convert TypeInfo to GenericType for field and method parsing
    std::shared_ptr<ast::GenericType> ClassParser::convertTypeInfoToGenericType(const TypeInfo& typeInfo)
    {
        // Handle basic types
        if (typeInfo.baseType != ValueType::OBJECT)
        {
            // Simple built-in types (int, float, bool, string, void)
            return std::make_shared<ast::GenericType>(typeInfo.baseType);
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
