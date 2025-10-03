#include "ConstructorParser.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../../ast/nodes/classes/ConstructorNode.hpp"
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

        auto body = context.parseStatement();

        // Use the new constructor that preserves ParameterType information
        return std::make_unique<ConstructorNode>(std::move(parametersWithTypes), std::move(body), constructorLocation);
    }
}
