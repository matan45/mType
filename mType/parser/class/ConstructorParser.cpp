#include "ConstructorParser.hpp"
#include "../utilities/ParserUtils.hpp"
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
        return stream.check(TokenType::CONSTRUCTOR);
    }

    std::unique_ptr<ASTNode> ConstructorParser::parseConstructor()
    {
        auto constructorLocation = tokenStream.current().location;
        tokenStream.expect(TokenType::CONSTRUCTOR);

        // Parse parameter list with full type information
        auto parametersWithTypes = ParserUtils::parseParameterListWithTypes(tokenStream, true);

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

        auto body = context.parseStatement();

        // Create constructor node
        auto constructorNode = std::make_unique<ConstructorNode>(
            std::move(parametersWithTypes), std::move(body), constructorLocation);

        // Set super initializer if present
        if (superInitializer)
        {
            constructorNode->setSuperInitializer(std::move(superInitializer));
        }

        return constructorNode;
    }
}
