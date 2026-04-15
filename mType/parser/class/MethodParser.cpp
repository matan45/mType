#include "MethodParser.hpp"
#include "GenericParameterParser.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../utilities/ParameterParser.hpp"
#include "../utilities/AccessModifierParser.hpp"
#include "../utilities/AsyncValidator.hpp"
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
        // Capture the method declaration start location
        SourceLocation methodStartLocation = tokenStream.current().location;

        // Parse access modifier first (default to PRIVATE for class methods)
        ast::AccessModifier accessModifier =
            utilities::AccessModifierParser::parseAccessModifier(tokenStream, ast::AccessModifier::PRIVATE);

        // Check for final modifier
        bool isFinal = false;
        if (tokenStream.current().type == TokenType::FINAL)
        {
            isFinal = true;
            tokenStream.advance();
        }

        // Check for abstract modifier
        bool isAbstract = false;
        if (tokenStream.current().type == TokenType::ABSTRACT)
        {
            isAbstract = true;
            tokenStream.advance();
        }

        bool isStatic = false;

        // Handle static modifier
        if (tokenStream.current().type == TokenType::STATIC)
        {
            isStatic = true;
            tokenStream.advance();
        }

        // Validate that abstract and final are mutually exclusive
        if (isAbstract && isFinal)
        {
            throw ParseException(
                "Method cannot be both abstract and final. "
                "Abstract methods must be overridden, while final methods cannot be overridden.",
                tokenStream.current().location);
        }

        // Validate that abstract and static are mutually exclusive
        if (isAbstract && isStatic)
        {
            throw ParseException(
                "Method cannot be both abstract and static. Only instance methods can be abstract.",
                tokenStream.current().location);
        }

        return parseMethodWithModifiers(accessModifier, isStatic, false, isAbstract, isFinal, methodStartLocation);
    }

    std::unique_ptr<ASTNode> MethodParser::parseStaticMethod()
    {
        // Capture the method declaration start location
        SourceLocation methodStartLocation = tokenStream.current().location;

        return parseMethodWithModifiers(ast::AccessModifier::PRIVATE, true, false, false, false, methodStartLocation);
    }

    std::unique_ptr<ASTNode> MethodParser::parseMethodWithModifiers(ast::AccessModifier accessModifier, bool isStatic,
                                                                    bool isAsync, bool isAbstract, bool isFinal,
                                                                    const SourceLocation& methodStartLocation)
    {
        // Handle function keyword (required for methods)
        if (tokenStream.current().type != TokenType::FUNCTION)
        {
            throw ParseException("Expected 'function' keyword", tokenStream.current().location);
        }
        tokenStream.advance();

        // NEW: Check for async keyword AFTER function keyword
        if (tokenStream.current().type == TokenType::ASYNC)
        {
            isAsync = true;
            tokenStream.advance();
        }

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

        // Parse parameter list using generic-aware utility (MYT-110: capture
        // per-parameter annotations via the Decl variant).
        auto paramDecls = ParameterParser::parseGenericParameterDeclList(tokenStream, true);
        std::vector<std::pair<std::string, std::shared_ptr<GenericType>>> parameters;
        std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> parameterAnnotations;
        parameters.reserve(paramDecls.size());
        parameterAnnotations.reserve(paramDecls.size());
        for (auto& d : paramDecls) {
            parameters.emplace_back(d.name, d.type);
            parameterAnnotations.push_back(std::move(d.annotations));
        }

        // Parse return type after the parameters using new generic type system
        std::shared_ptr<GenericType> returnType = std::make_shared<GenericType>(ValueType::VOID);
        if (tokenStream.current().type == TokenType::COLON)
        {
            tokenStream.advance();
            returnType = TypeParser::parseGenericType(tokenStream);
        }

        // Validate async method return type must be Promise<T>
        if (isAsync)
        {
            utilities::AsyncValidator::validateAsyncReturnType(returnType, tokenStream.location());
        }

        // Parse method body (abstract methods have no body)
        std::unique_ptr<ASTNode> body = nullptr;

        if (isAbstract)
        {
            // Abstract methods must end with semicolon (no body)
            if (tokenStream.current().type != TokenType::SEMICOLON)
            {
                throw ParseException(
                    "Abstract method '" + methodName + "' must not have a body. "
                    "Expected semicolon after method signature.",
                    tokenStream.current().location);
            }
            tokenStream.advance(); // consume semicolon
        }
        else
        {
            // Non-abstract methods must have a body
            ParseContext::AsyncContextGuard asyncGuard(context.getContextState(), isAsync);
            body = context.parseStatement();
        }

        // Create MethodNode with generic support, access modifier, async flag, and location
        // Use the location from the start of the method declaration (passed as parameter)
        auto methodNode = std::make_unique<MethodNode>(methodName, returnType, std::move(parameters),
                                                       std::move(body), isStatic, methodGenericParameters,
                                                       accessModifier, isAsync, methodStartLocation);
        methodNode->setAbstract(isAbstract);
        methodNode->setFinal(isFinal);
        methodNode->setParameterAnnotations(std::move(parameterAnnotations));
        return methodNode;
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

            tokenStream.expectGreaterForGeneric(); // consume '>' (handles >> for nested generics in constraints)
        }
        return methodGenericParameters;
    }

    void MethodParser::validateMethodName(const std::string& methodName, bool isStatic)
    {
        std::string methodType = isStatic ? "Static method" : "Method";
        ParserUtils::validateFunctionNamingConvention(methodName, isStatic, methodType, tokenStream.location());
    }
}
