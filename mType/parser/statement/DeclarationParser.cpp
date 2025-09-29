#include "DeclarationParser.hpp"
#include "../TypeParser.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../exceptions/DomainExceptions.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::statement
{
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;
    using namespace token;
    using namespace errors;

    std::unique_ptr<ASTNode> DeclarationParser::parse()
    {
        return parseDeclaration();
    }

    bool DeclarationParser::canParse(const TokenStream& stream) const
    {
        return isDeclarationStart(stream.current().type);
    }

    std::unique_ptr<ASTNode> DeclarationParser::parseDeclaration()
    {
        // Parse modifiers (final, static)
        ModifierInfo modifiers = parseModifiers();

        // Parse the complete type information
        auto typeLocation = getCurrentLocation();
        parser::TypeInfo typeInfo = TypeParser::parseTypeInfo(tokenStream);
        ValueType type = typeInfo.baseType;
        std::string className = typeInfo.className;

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            // Special case: if we see a parenthesis after a qualified name,
            // it's likely a static method call that was mistakenly routed here
            if (tokenStream.check(TokenType::LPAREN) && !className.empty() &&
                className.find("::") != std::string::npos)
            {
                // This is actually a static method call (e.g., Class::method())
                // Parse it as a function call instead of a declaration

                tokenStream.advance(); // consume '('
                std::vector<std::unique_ptr<ASTNode>> arguments;

                if (!tokenStream.check(TokenType::RPAREN))
                {
                    arguments.push_back(context.parseExpression());
                    while (tryConsumeToken(TokenType::COMMA))
                    {
                        arguments.push_back(context.parseExpression());
                    }
                }

                expectToken(TokenType::RPAREN, getParserName());
                expectToken(TokenType::SEMICOLON, getParserName());

                // Create a FunctionCallNode with the qualified name
                return std::make_unique<FunctionCallNode>(className, std::move(arguments), typeLocation);
            }

            reportError("Expected variable name in declaration", getParserName());
            throw errors::ParseException("Expected variable name");
        }

        std::string varName = tokenStream.current().stringValue.getString();
        SourceLocation varLocation = getCurrentLocation();
        tokenStream.advance();

        std::unique_ptr<ASTNode> value = nullptr;
        if (tryConsumeToken(TokenType::ASSIGN))
        {
            value = context.parseExpression();
        }

        expectToken(TokenType::SEMICOLON, getParserName());

        return std::make_unique<AssignmentNode>(varName, std::move(value), type, className,
                                              modifiers.isFinal, modifiers.isStatic, varLocation);
    }

    bool DeclarationParser::isDeclarationStart(TokenType type) const noexcept
    {
        return isModifierToken(type) || isTypeToken(type) || type == TokenType::IDENTIFIER;
    }

    bool DeclarationParser::isModifierToken(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::FINAL:
        case TokenType::STATIC:
            return true;
        default:
            return false;
        }
    }

    bool DeclarationParser::isTypeToken(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::INT:
        case TokenType::FLOAT:
        case TokenType::BOOL:
        case TokenType::STRING_TYPE:
        case TokenType::VOID:
            return true;
        default:
            return false;
        }
    }

    DeclarationParser::ModifierInfo DeclarationParser::parseModifiers()
    {
        ModifierInfo modifiers;

        // Handle final modifier
        if (tryConsumeToken(TokenType::FINAL))
        {
            modifiers.isFinal = true;
        }

        // Handle static modifier
        if (tryConsumeToken(TokenType::STATIC))
        {
            modifiers.isStatic = true;
        }

        return modifiers;
    }
}