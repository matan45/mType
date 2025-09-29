#include "FieldParser.hpp"
#include "../TypeParser.hpp"
#include "../ParserUtils.hpp"
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

    FieldParser::FieldParser(TokenStream& tokenStream, ParseContext& context)
        : tokenStream(tokenStream), context(context)
    {
    }

    std::unique_ptr<ASTNode> FieldParser::parse()
    {
        return parseField();
    }

    bool FieldParser::canParse(const TokenStream& stream) const
    {
        return stream.check(TokenType::STATIC) ||
               stream.check(TokenType::FINAL) ||
               stream.check(TokenType::INT) ||
               stream.check(TokenType::FLOAT) ||
               stream.check(TokenType::BOOL) ||
               stream.check(TokenType::STRING_TYPE) ||
               stream.check(TokenType::IDENTIFIER);
    }

    std::string FieldParser::getParserName() const
    {
        return "FieldParser";
    }

    std::unique_ptr<ASTNode> FieldParser::parseField()
    {
        auto [isStatic, isFinal] = parseFieldModifiers();

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
            std::vector<ast::GenericTypeParameter> methodGenericParameters;
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
            std::shared_ptr<ast::GenericType> returnType = std::make_shared<ast::GenericType>(ValueType::VOID);
            if (tokenStream.current().type == TokenType::COLON)
            {
                tokenStream.advance();
                returnType = TypeParser::parseGenericType(tokenStream);
            }

            // Parse method body
            auto body = context.parseStatement();

            // Create MethodNode with generic support
            return std::make_unique<MethodNode>(methodName, returnType, std::move(parameters),
                                                std::move(body), isStatic, methodGenericParameters);
        }

        return parseFieldDeclaration(isStatic, isFinal);
    }

    std::pair<bool, bool> FieldParser::parseFieldModifiers()
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

        return {isStatic, isFinal};
    }

    std::unique_ptr<ASTNode> FieldParser::parseFieldDeclaration(bool isStatic, bool isFinal)
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
                                           isStatic, isFinal);
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
