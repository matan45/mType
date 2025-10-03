#include "MethodParser.hpp"
#include "GenericParameterParser.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../utilities/AccessModifierParser.hpp"
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

    MethodParser::MethodParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }

    std::unique_ptr<ASTNode> MethodParser::parse()
    {
        return parseMethod();
    }

    bool MethodParser::canParse(const TokenStream& stream) const
    {
        // Check for access modifiers first
        if (stream.check(TokenType::PRIVATE) || stream.check(TokenType::PUBLIC) || stream.check(TokenType::PROTECTED))
        {
            return true;
        }

        return stream.check(TokenType::FUNCTION) ||
            (stream.check(TokenType::STATIC) && stream.peekAhead(1).type == TokenType::FUNCTION);
    }

    std::unique_ptr<ASTNode> MethodParser::parseMethod()
    {
        // Parse access modifier first (default to PRIVATE for class methods)
        ast::AccessModifier accessModifier =
            utilities::AccessModifierParser::parseAccessModifier(tokenStream, ast::AccessModifier::PRIVATE);

        bool isStatic = false;

        // Handle static modifier
        if (tokenStream.current().type == TokenType::STATIC)
        {
            isStatic = true;
            tokenStream.advance();
        }

        return parseMethodWithModifiers(accessModifier, isStatic);
    }

    std::unique_ptr<ASTNode> MethodParser::parseStaticMethod()
    {
        return parseMethodWithModifiers(ast::AccessModifier::PRIVATE, true);
    }

    std::unique_ptr<ASTNode> MethodParser::parseMethodWithModifiers(ast::AccessModifier accessModifier, bool isStatic)
    {
        // Handle function keyword (required for methods)
        if (tokenStream.current().type != TokenType::FUNCTION)
        {
            throw ParseException("Expected 'function' keyword", tokenStream.current().location);
        }
        tokenStream.advance();

        // Parse generic type parameters for generic methods
        std::vector<GenericTypeParameter> methodGenericParameters = parseMethodGenericParameters();

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
        std::shared_ptr<GenericType> returnType = std::make_shared<GenericType>(ValueType::VOID);
        if (tokenStream.current().type == TokenType::COLON)
        {
            tokenStream.advance();
            returnType = TypeParser::parseGenericType(tokenStream);
        }

        auto body = context.parseStatement();

        // Create MethodNode with generic support and access modifier
        return std::make_unique<MethodNode>(methodName, returnType, std::move(parameters),
                                            std::move(body), isStatic, methodGenericParameters, accessModifier);
    }

    std::vector<GenericTypeParameter> MethodParser::parseMethodGenericParameters()
    {
        std::vector<GenericTypeParameter> methodGenericParameters;
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
