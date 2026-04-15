#include "ConstructorParser.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../utilities/ParameterParser.hpp"
#include "../utilities/AccessModifierParser.hpp"
#include "../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../ast/nodes/classes/SuperConstructorCallNode.hpp"
#include "../../errors/ParseException.hpp"
#include <utility>

namespace parser
{
    using namespace ast::nodes::classes;
    using namespace token;
    using namespace errors;

    ConstructorParser::ConstructorParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }

    std::unique_ptr<ASTNode> ConstructorParser::parse()
    {
        return parseConstructor();
    }

    bool ConstructorParser::canParse(const TokenStream& stream) const
    {
        // Check for access modifiers or constructor keyword
        return stream.check(TokenType::PRIVATE) ||
            stream.check(TokenType::PUBLIC) ||
            stream.check(TokenType::PROTECTED) ||
            stream.check(TokenType::CONSTRUCTOR);
    }

    std::unique_ptr<ASTNode> ConstructorParser::parseConstructor()
    {
        // Parse access modifier first (default to PUBLIC for constructors)
        ast::AccessModifier accessModifier =
            utilities::AccessModifierParser::parseAccessModifier(tokenStream, ast::AccessModifier::PUBLIC);

        auto constructorLocation = tokenStream.current().location;
        tokenStream.expect(TokenType::CONSTRUCTOR);

        // Parse parameter list with full type information. MYT-110: Decl
        // variant collects per-parameter annotations.
        auto paramDecls = ParameterParser::parseTypedParameterDeclList(tokenStream, true);
        std::vector<std::pair<std::string, value::ParameterType>> parametersWithTypes;
        std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> parameterAnnotations;
        parametersWithTypes.reserve(paramDecls.size());
        parameterAnnotations.reserve(paramDecls.size());
        for (auto& d : paramDecls) {
            parametersWithTypes.emplace_back(d.name, d.type);
            parameterAnnotations.push_back(std::move(d.annotations));
        }

        // Check for super initializer: `: super(...)`
        std::unique_ptr<SuperConstructorCallNode> superInitializer = nullptr;
        if (tokenStream.check(TokenType::COLON))
        {
            tokenStream.advance(); // consume ':'

            if (!tokenStream.check(TokenType::SUPER))
            {
                throw ParseException(
                    "Expected 'super' after ':' in constructor initializer list",
                    tokenStream.current().location);
            }

            auto superLocation = tokenStream.current().location;
            tokenStream.advance(); // consume 'super'

            if (!tokenStream.check(TokenType::LPAREN))
            {
                throw ParseException(
                    "Expected '(' after 'super' in constructor initializer",
                    tokenStream.current().location);
            }

            tokenStream.advance(); // consume '('

            std::vector<std::unique_ptr<ASTNode>> arguments;

            // Parse arguments if not empty
            if (!tokenStream.check(TokenType::RPAREN))
            {
                arguments.push_back(context.parseExpression());
                while (tokenStream.check(TokenType::COMMA))
                {
                    tokenStream.advance(); // consume ','
                    arguments.push_back(context.parseExpression());
                }
            }

            tokenStream.expect(TokenType::RPAREN);

            superInitializer = std::make_unique<SuperConstructorCallNode>(
                std::move(arguments), superLocation);
        }

        // Set constructor context when parsing constructor body
        ParseContext::ConstructorContextGuard constructorGuard(context.getContextState());
        auto body = context.parseStatement();

        // Create constructor node with parsed access modifier
        auto constructorNode = std::make_unique<ConstructorNode>(
            std::move(parametersWithTypes), std::move(body),
            accessModifier, constructorLocation);

        // Set super initializer if present
        if (superInitializer)
        {
            constructorNode->setSuperInitializer(std::move(superInitializer));
        }

        // MYT-110: attach per-parameter annotations
        constructorNode->setParameterAnnotations(std::move(parameterAnnotations));

        return constructorNode;
    }
}
