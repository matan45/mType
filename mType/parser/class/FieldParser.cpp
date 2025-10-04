#include "FieldParser.hpp"
#include "../TypeParser.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../utilities/AccessModifierParser.hpp"
#include "../../ast/nodes/classes/FieldNode.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../errors/ParseException.hpp"
#include <utility>

#include "GenericParameterParser.hpp"

namespace parser
{
    using namespace ast::nodes::classes;
    using namespace token;
    using namespace value;
    using namespace errors;

    FieldParser::FieldParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }

    std::unique_ptr<ASTNode> FieldParser::parse()
    {
        return parseField();
    }

    bool FieldParser::canParse(const TokenStream& stream) const
    {
        return stream.check(TokenType::PRIVATE) ||
            stream.check(TokenType::PUBLIC) ||
            stream.check(TokenType::PROTECTED) ||
            stream.check(TokenType::STATIC) ||
            stream.check(TokenType::FINAL) ||
            stream.check(TokenType::INT) ||
            stream.check(TokenType::FLOAT) ||
            stream.check(TokenType::BOOL) ||
            stream.check(TokenType::STRING_TYPE) ||
            stream.check(TokenType::IDENTIFIER);
    }

    std::unique_ptr<ASTNode> FieldParser::parseField()
    {
        auto [accessModifier, isStatic, isFinal] = parseFieldModifiers();

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

            // Parse generic type parameters for static methods
            std::vector<GenericTypeParameter> methodGenericParameters;
            if (tokenStream.check(TokenType::LESS))
            {
                tokenStream.advance(); // consume '<'

                // Use GenericParameterParser to properly parse the generic type parameters
                GenericParameterParser genericParser(tokenStream, context);
                methodGenericParameters = genericParser.parseGenericTypeParameters();

                tokenStream.expect(TokenType::GREATER); // consume '>'
            }

            // Parse method name
            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected method name", tokenStream.current().location);
            }

            std::string methodName = tokenStream.current().stringValue.getString();

            // Validate static method naming convention
            ParserUtils::validateFunctionNamingConvention(methodName, true, "Static method", tokenStream.location());

            tokenStream.advance();

            // Parse parameter list using generic-aware utility
            auto parameters = ParserUtils::parseGenericParameterList(tokenStream, true);

            // Parse return type using generic type system
            std::shared_ptr<GenericType> returnType = std::make_shared<ast::GenericType>(ValueType::VOID);
            if (tokenStream.current().type == TokenType::COLON)
            {
                tokenStream.advance();
                returnType = TypeParser::parseGenericType(tokenStream);
            }

            // Parse method body
            auto body = context.parseStatement();

            // Create MethodNode with generic support and access modifier
            return std::make_unique<MethodNode>(methodName, returnType, std::move(parameters),
                                                std::move(body), isStatic, methodGenericParameters, accessModifier);
        }

        return parseFieldDeclaration(accessModifier, isStatic, isFinal);
    }

    std::tuple<ast::AccessModifier, bool, bool> FieldParser::parseFieldModifiers()
    {
        // Parse access modifier first (default to PRIVATE for class fields)
        ast::AccessModifier accessModifier =
            utilities::AccessModifierParser::parseAccessModifier(tokenStream, ast::AccessModifier::PRIVATE);

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

        return {accessModifier, isStatic, isFinal};
    }

    std::unique_ptr<ASTNode> FieldParser::parseFieldDeclaration(ast::AccessModifier accessModifier, bool isStatic, bool isFinal)
    {
        // Parse the complete type information using TypeParser
        auto fieldGenericType = TypeParser::parseGenericType(tokenStream);

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected field name", tokenStream.current().location);
        }

        std::string fieldName = tokenStream.current().stringValue.getString();
        tokenStream.advance();

        std::unique_ptr<ASTNode> initialValue = parseInitialValue();

        tokenStream.expect(TokenType::SEMICOLON);

        return std::make_unique<FieldNode>(fieldName, fieldGenericType, std::move(initialValue),
                                           isStatic, isFinal, accessModifier);
    }

    std::unique_ptr<ASTNode> FieldParser::parseInitialValue()
    {
        std::unique_ptr<ASTNode> initialValue = nullptr;
        if (tokenStream.current().type == TokenType::ASSIGN)
        {
            tokenStream.advance();
            initialValue = context.parseExpression();
        }
        return initialValue;
    }
}
