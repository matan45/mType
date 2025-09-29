#include "ClassParser.hpp"
#include "class/ClassDeclarationParser.hpp"
#include "class/ConstructorParser.hpp"
#include "class/MethodParser.hpp"
#include "class/FieldParser.hpp"
#include "class/ObjectCreationParser.hpp"
#include "class/GenericParameterParser.hpp"
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

    ClassParser::ClassParser(TokenStream& stream, ParseContext& ctx)
        : tokenStream(stream), context(ctx)
    {
        initializeHelperParsers();
    }

    ClassParser::~ClassParser() = default;

    void ClassParser::initializeHelperParsers()
    {
        classDeclarationParser = std::make_unique<ClassDeclarationParser>(tokenStream, context);
        constructorParser = std::make_unique<ConstructorParser>(tokenStream, context);
        methodParser = std::make_unique<MethodParser>(tokenStream, context);
        fieldParser = std::make_unique<FieldParser>(tokenStream, context);
        objectCreationParser = std::make_unique<ObjectCreationParser>(tokenStream, context);
        genericParameterParser = std::make_unique<GenericParameterParser>(tokenStream, context);
    }

    std::unique_ptr<ASTNode> ClassParser::parseClass()
    {
        // Delegate to ClassDeclarationParser for class header parsing
        auto classNode = classDeclarationParser->parseClassDeclaration();
        auto* classNodePtr = dynamic_cast<ClassNode*>(classNode.get());

        if (!classNodePtr)
        {
            throw ParseException("Failed to create class node", tokenStream.current().location);
        }

        // Parse class body members
        while (tokenStream.current().type != TokenType::RBRACE && tokenStream.current().type != TokenType::END)
        {
            TokenType currentToken = tokenStream.current().type;

            if (currentToken == TokenType::CONSTRUCTOR)
            {
                auto constructor = constructorParser->parseConstructor();
                if (constructor)
                {
                    classNodePtr->addConstructor(std::move(constructor));
                }
            }
            else if (currentToken == TokenType::FUNCTION ||
                     (currentToken == TokenType::STATIC && tokenStream.peekAhead(1).type == TokenType::FUNCTION))
            {
                auto method = methodParser->parseMethod();
                if (method)
                {
                    classNodePtr->addMethod(std::move(method));
                }
            }
            else
            {
                // Default case - parse as field (handles static/final modifiers and static methods)
                auto field = fieldParser->parseField();
                if (field)
                {
                    // Check if the parsed field is actually a method (static function)
                    if (dynamic_cast<MethodNode*>(field.get()))
                    {
                        classNodePtr->addMethod(std::move(field));
                    }
                    else
                    {
                        classNodePtr->addField(std::move(field));
                    }
                }
            }
        }

        tokenStream.expect(TokenType::RBRACE);
        return classNode;
    }

    std::unique_ptr<ASTNode> ClassParser::parseConstructor()
    {
        return constructorParser->parseConstructor();
    }

    std::unique_ptr<ASTNode> ClassParser::parseMethod()
    {
        return methodParser->parseMethod();
    }

    std::unique_ptr<ASTNode> ClassParser::parseField()
    {
        return fieldParser->parseField();
    }

    std::unique_ptr<ASTNode> ClassParser::parseNewExpression()
    {
        return objectCreationParser->parseNewExpression();
    }

    std::string ClassParser::parseGenericParameters()
    {
        return genericParameterParser->parseGenericParameters();
    }

    std::string ClassParser::parseGenericParameter()
    {
        return genericParameterParser->parseGenericParameter();
    }

    std::vector<ast::GenericTypeParameter> ClassParser::parseGenericTypeParameters()
    {
        return genericParameterParser->parseGenericTypeParameters();
    }

    ast::GenericTypeParameter ClassParser::parseGenericTypeParameter()
    {
        return genericParameterParser->parseGenericTypeParameter();
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
            std::string genericPart = className.substr(anglePos + 1, className.length() - anglePos - 2);
            // Remove < and >

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
