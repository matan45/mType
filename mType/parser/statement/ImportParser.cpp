#include "ImportParser.hpp"
#include "../../ast/nodes/statements/ImportNode.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::statement
{
    using namespace ast::nodes::statements;
    using namespace token;
    using namespace errors;

    ImportParser::ImportParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }


    std::unique_ptr<ASTNode> ImportParser::parse()
    {
        return parseImport();
    }

    bool ImportParser::canParse(const TokenStream& stream) const
    {
        return isImportToken(stream.current().type);
    }

    std::unique_ptr<ASTNode> ImportParser::parseImport()
    {
        expectToken(TokenType::IMPORT);

        if (!tokenStream.check(TokenType::STRING_LITERAL))
        {
            throw ParseException("Expected string literal after 'import'", tokenStream.current().location);
        }

        std::string filePath = tokenStream.current().stringValue.getString();
        SourceLocation loc = tokenStream.current().location;
        tokenStream.advance();

        validateImportPath(filePath);

        expectToken(TokenType::SEMICOLON);

        // Create a pure import node - no processing, no ImportManager dependency
        auto importNode = std::make_unique<ImportNode>(filePath, loc);

        // No processing at parse time - completely defer to evaluation phase
        return importNode;
    }

    bool ImportParser::isImportToken(TokenType type) const noexcept
    {
        return type == TokenType::IMPORT;
    }

    void ImportParser::validateImportPath(const std::string& path)
    {
        if (path.empty())
        {
            throw ParseException("Import path cannot be empty", tokenStream.current().location);
        }
    }
}
