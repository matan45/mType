#include "FieldParser.hpp"
#include "../TypeParser.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../utilities/AccessModifierParser.hpp"
#include "../../ast/nodes/classes/FieldNode.hpp"
#include "../../errors/ParseException.hpp"
#include <utility>

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

        // FieldParser should only handle field declarations
        // Method declarations should be detected and handled by ClassParser
        if (tokenStream.current().type == TokenType::FUNCTION)
        {
            throw ParseException("Unexpected 'function' keyword in field declaration context. "
                                 "This should have been handled by MethodParser.",
                                 tokenStream.current().location);
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

    std::unique_ptr<ASTNode> FieldParser::parseFieldDeclaration(ast::AccessModifier accessModifier, bool isStatic,
                                                                bool isFinal)
    {
        // Parse the complete type information using TypeParser
        auto fieldGenericType = TypeParser::parseGenericType(tokenStream);

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected field name", tokenStream.current().location);
        }

        std::string fieldName = std::string(tokenStream.current().stringValue);
        SourceLocation fieldLocation = tokenStream.current().location;

        // Validate field name contains no special characters
        ParserUtils::validateIdentifierName(fieldName, "Field", fieldLocation);

        tokenStream.advance();

        std::unique_ptr<ASTNode> initialValue = parseInitialValue();

        tokenStream.expect(TokenType::SEMICOLON);

        return std::make_unique<FieldNode>(fieldName, fieldGenericType, std::move(initialValue),
                                           isStatic, isFinal, accessModifier, fieldLocation);
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
