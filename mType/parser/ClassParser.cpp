#include "ClassParser.hpp"
#include "class/ClassDeclarationParser.hpp"
#include "class/ConstructorParser.hpp"
#include "class/MethodParser.hpp"
#include "class/FieldParser.hpp"
#include "class/ObjectCreationParser.hpp"
#include "class/GenericParameterParser.hpp"
#include "utilities/ParserUtils.hpp"
#include "../services/ImportManager.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
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

    std::vector<GenericTypeParameter> ClassParser::parseGenericTypeParameters()
    {
        return genericParameterParser->parseGenericTypeParameters();
    }

    GenericTypeParameter ClassParser::parseGenericTypeParameter()
    {
        return genericParameterParser->parseGenericTypeParameter();
    }
}
