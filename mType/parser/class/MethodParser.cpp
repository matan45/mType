#include "MethodParser.hpp"
#include "GenericParameterParser.hpp"
#include "../ParserUtils.hpp"
#include "../TypeParser.hpp"
#include "../../ast/nodes/classes/MethodNode.hpp"
#include "../../errors/ParseException.hpp"
#include <utility>

namespace parser
{
    using namespace ast::nodes::classes;
    using namespace token;
    using namespace value;
    using namespace errors;

    MethodParser::MethodParser(TokenStream& tokenStream, ParseContext& context)
        : tokenStream(tokenStream), context(context)
    {
    }

    std::unique_ptr<ASTNode> MethodParser::parse()
    {
        return parseMethod();
    }

    bool MethodParser::canParse(const TokenStream& stream) const
    {
        return stream.check(TokenType::FUNCTION) ||
               (stream.check(TokenType::STATIC) && stream.peekAhead(1).type == TokenType::FUNCTION);
    }

    std::string MethodParser::getParserName() const
    {
        return "MethodParser";
    }

    std::unique_ptr<ASTNode> MethodParser::parseMethod()
    {
        bool isStatic = false;

        // Handle static modifier
        if (tokenStream.current().type == TokenType::STATIC)
        {
            isStatic = true;
            tokenStream.advance();
        }

        return parseMethodWithModifiers(isStatic);
    }

    std::unique_ptr<ASTNode> MethodParser::parseStaticMethod()
    {
        return parseMethodWithModifiers(true);
    }

    std::unique_ptr<ASTNode> MethodParser::parseMethodWithModifiers(bool isStatic)
    {
        // Handle function keyword (required for methods)
        if (tokenStream.current().type != TokenType::FUNCTION)
        {
            throw ParseException("Expected 'function' keyword", tokenStream.current().location);
        }
        tokenStream.advance();

        // Parse generic type parameters for generic methods
        std::vector<ast::GenericTypeParameter> methodGenericParameters = parseMethodGenericParameters();

        // Parse method name
        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected method name", tokenStream.current().location);
        }

        std::string methodName = tokenStream.current().stringValue.getString();
        validateMethodName(methodName, isStatic);
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

        // Create MethodNode with generic support
        return std::make_unique<MethodNode>(methodName, returnType, std::move(parameters),
                                            std::move(body), isStatic, methodGenericParameters);
    }

    std::vector<ast::GenericTypeParameter> MethodParser::parseMethodGenericParameters()
    {
        std::vector<ast::GenericTypeParameter> methodGenericParameters;
        if (tokenStream.check(TokenType::LESS))
        {
            tokenStream.advance(); // consume '<'

            // Use GenericParameterParser to properly parse the generic type parameters
            GenericParameterParser genericParser(tokenStream, context);
            methodGenericParameters = genericParser.parseGenericTypeParameters();

            tokenStream.expect(TokenType::GREATER); // consume '>'
        }
        return methodGenericParameters;
    }

    void MethodParser::validateMethodName(const std::string& methodName, bool isStatic)
    {
        std::string methodType = isStatic ? "Static method" : "Method";
        ParserUtils::validateFunctionNamingConvention(methodName, isStatic, methodType, tokenStream.location());
    }
}