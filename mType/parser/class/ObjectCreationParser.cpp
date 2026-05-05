#include "ObjectCreationParser.hpp"
#include <cstddef>
#include "../ParserValidator.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../expression/ArgumentParser.hpp"
#include "../../ast/nodes/classes/NewNode.hpp"
#include "../../ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../../errors/ParseException.hpp"
#include <utility>

namespace parser
{
    using namespace ast::nodes::classes;
    using namespace ast::nodes::expressions;
    using namespace token;
    using namespace value;
    using namespace errors;

    ObjectCreationParser::ObjectCreationParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }

    std::unique_ptr<ASTNode> ObjectCreationParser::parse()
    {
        return parseNewExpression();
    }

    bool ObjectCreationParser::canParse(const TokenStream& stream) const
    {
        return stream.check(TokenType::NEW);
    }

    std::unique_ptr<ASTNode> ObjectCreationParser::parseNewExpression()
    {
        auto newLocation = tokenStream.current().location;
        tokenStream.expect(TokenType::NEW);

        // Check for valid type names
        TokenType currentType = tokenStream.current().type;
        if (currentType != TokenType::IDENTIFIER &&
            currentType != TokenType::INT &&
            currentType != TokenType::FLOAT &&
            currentType != TokenType::BOOL &&
            currentType != TokenType::STRING_TYPE)
        {
            throw ParseException("Expected class name after 'new'", tokenStream.current().location);
        }

        std::string className = parseClassNameWithGenerics();

        // Check if this is array creation or class instantiation
        if (tokenStream.check(TokenType::LBRACKET))
        {
            return parseArrayCreation(className);
        }

        return parseObjectInstantiation(className);
    }

    std::unique_ptr<ASTNode> ObjectCreationParser::parseArrayCreation(const std::string& className)
    {
        std::vector<std::unique_ptr<ASTNode>> sizeExpressions = parseArrayDimensions();
        TypeInfo elementTypeInfo = createElementTypeInfo(className);
        auto newLocation = tokenStream.current().location;

        return std::make_unique<ArrayCreationNode>(elementTypeInfo, std::move(sizeExpressions), newLocation);
    }

    std::unique_ptr<ASTNode> ObjectCreationParser::parseObjectInstantiation(const std::string& className)
    {
        // Extract final class name for validation
        std::vector<std::string> qualifiedParts;
        size_t scopePos = className.find("::");
        if (scopePos != std::string::npos)
        {
            qualifiedParts.push_back(className.substr(scopePos + 2));
        }
        else
        {
            qualifiedParts.push_back(className);
        }

        std::string finalClassName = qualifiedParts.back();
        validateClassNameForInstantiation(finalClassName);

        std::vector<std::unique_ptr<ASTNode>> arguments = parseConstructorArguments();
        auto newLocation = tokenStream.current().location;

        return std::make_unique<NewNode>(className, std::move(arguments), newLocation);
    }

    std::string ObjectCreationParser::parseClassNameWithGenerics()
    {
        // Parse qualified class name
        std::vector<std::string> qualifiedParts;
        qualifiedParts.push_back(std::string(tokenStream.current().stringValue));
        tokenStream.advance();

        while (tokenStream.current().type == TokenType::SCOPE)
        {
            tokenStream.advance();
            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected identifier after '::'", tokenStream.current().location);
            }
            qualifiedParts.push_back(std::string(tokenStream.current().stringValue));
            tokenStream.advance();
        }

        // Reconstruct full qualified name
        std::string className = qualifiedParts[0];
        for (size_t i = 1; i < qualifiedParts.size(); ++i)
        {
            className += "::" + qualifiedParts[i];
        }

        // Handle generic parameters with proper nested bracket matching
        if (tokenStream.check(TokenType::LESS))
        {
            std::string genericsString = ParserUtils::parseNestedGenericExpression(tokenStream);
            className += genericsString;
        }

        return className;
    }

    std::vector<std::unique_ptr<ASTNode>> ObjectCreationParser::parseConstructorArguments()
    {
        expression::ArgumentParser argParser(tokenStream, context);
        return argParser.parseArgumentsWithParentheses();
    }

    std::vector<std::unique_ptr<ASTNode>> ObjectCreationParser::parseArrayDimensions()
    {
        std::vector<std::unique_ptr<ASTNode>> sizeExpressions;

        // Parse all dimensions
        while (tokenStream.check(TokenType::LBRACKET))
        {
            tokenStream.advance(); // consume '['

            // Check for empty brackets (jagged arrays)
            if (tokenStream.check(TokenType::RBRACKET))
            {
                sizeExpressions.push_back(nullptr);
            }
            else
            {
                auto sizeExpression = context.parseExpression();
                sizeExpressions.push_back(std::move(sizeExpression));
            }

            tokenStream.expect(TokenType::RBRACKET); // consume ']'
        }

        return sizeExpressions;
    }

    TypeInfo ObjectCreationParser::createElementTypeInfo(const std::string& className)
    {
        // Handle basic types
        if (className == "int") return TypeInfo(ValueType::INT);
        if (className == "float") return TypeInfo(ValueType::FLOAT);
        if (className == "bool") return TypeInfo(ValueType::BOOL);
        if (className == "string") return TypeInfo(ValueType::STRING);

        // Generic type parameter or custom class
        return TypeInfo(ValueType::OBJECT, className);
    }

    void ObjectCreationParser::validateClassNameForInstantiation(const std::string& finalClassName)
    {
        if (!ParserValidator::isValidClassName(finalClassName))
        {
            throw ParseException("Class name '" + finalClassName + "' must start with an uppercase letter",
                                 tokenStream.current().location);
        }
    }
}
