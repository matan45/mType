#include "ImportParser.hpp"
#include "../../ast/nodes/statements/ImportNode.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::statement
{
    using namespace ast::nodes::statements;
    using namespace token;
    using namespace errors;

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
        expectToken(TokenType::IMPORT, getParserName());

        if (!tokenStream.check(TokenType::STRING_LITERAL))
        {
            reportError("Expected string literal after 'import'", getParserName());
            throw errors::ParseException("Expected string literal after 'import'");
        }

        std::string filePath = tokenStream.current().stringValue.getString();
        SourceLocation loc = getCurrentLocation();
        tokenStream.advance();

        validateImportPath(filePath);

        expectToken(TokenType::SEMICOLON, getParserName());

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
            reportError("Import path cannot be empty", getParserName());
            throw errors::ParseException("Import path cannot be empty");
        }

        // Check for file extension
        if (path.find(".mt") == std::string::npos)
        {
            reportWarning("Import path should typically end with '.mt'", getParserName());
        }
    }
}