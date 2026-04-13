#include "ImportParser.hpp"
#include "../utilities/ParserUtils.hpp"
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
        SourceLocation loc = tokenStream.current().location;
        expectToken(TokenType::IMPORT);

        // Check for wildcard import: import * from "file.mt"
        if (tokenStream.check(TokenType::MULTIPLY))
        {
            tokenStream.advance(); // consume *
            return parseWildcardImport(loc);
        }

        // Check for library import: import lib "name" or import lib {A, B} from "name"
        if (tokenStream.check(TokenType::IDENTIFIER) &&
            tokenStream.current().stringValue.getString() == "lib")
        {
            tokenStream.advance(); // consume 'lib'
            return parseLibraryImport(loc);
        }

        // Check for selective import: import { A, B, C } from "file.mt"
        if (tokenStream.check(TokenType::LBRACE))
        {
            return parseSelectiveImport(loc);
        }

        throw ParseException(
            "Expected '{' for selective import, '*' for wildcard import, or 'lib' for library import after 'import' keyword. "
            "Syntax: 'import {Symbol} from \"file.mt\"', 'import * from \"file.mt\"', or 'import lib \"name\"'",
            tokenStream.current().location);
    }

    std::unique_ptr<ASTNode> ImportParser::parseSelectiveImport(const SourceLocation& loc)
    {
        expectToken(TokenType::LBRACE); // {

        std::vector<std::string> symbols;

        // Parse symbol list: A, B, C
        while (!tokenStream.check(TokenType::RBRACE))
        {
            if (!tokenStream.check(TokenType::IDENTIFIER))
            {
                throw ParseException("Expected identifier in import list", tokenStream.current().location);
            }

            std::string symbolName = tokenStream.current().stringValue.getString();
            SourceLocation symbolLocation = tokenStream.current().location;

            // Validate symbol name contains no special characters
            ParserUtils::validateIdentifierName(symbolName, "Import symbol", symbolLocation);

            symbols.push_back(symbolName);
            tokenStream.advance();

            if (tokenStream.check(TokenType::COMMA))
            {
                tokenStream.advance(); // consume comma
            }
            else if (!tokenStream.check(TokenType::RBRACE))
            {
                throw ParseException("Expected ',' or '}' in import list", tokenStream.current().location);
            }
        }

        expectToken(TokenType::RBRACE); // }

        if (symbols.empty())
        {
            throw ParseException("Import list cannot be empty", loc);
        }

        // Expect 'from' keyword
        expectToken(TokenType::FROM);

        // Expect file path string
        if (!tokenStream.check(TokenType::STRING_LITERAL))
        {
            throw ParseException("Expected string literal after 'from'", tokenStream.current().location);
        }

        std::string filePath = tokenStream.current().stringValue.getString();
        tokenStream.advance();

        validateImportPath(filePath);

        expectToken(TokenType::SEMICOLON);

        // Create selective import node
        return std::make_unique<ImportNode>(filePath, ImportType::SELECTIVE, symbols, loc);
    }

    std::unique_ptr<ASTNode> ImportParser::parseWildcardImport(const SourceLocation& loc)
    {
        // Expect 'from' keyword
        expectToken(TokenType::FROM);

        // Expect file path string
        if (!tokenStream.check(TokenType::STRING_LITERAL))
        {
            throw ParseException("Expected string literal after 'from'", tokenStream.current().location);
        }

        std::string filePath = tokenStream.current().stringValue.getString();
        tokenStream.advance();

        validateImportPath(filePath);

        expectToken(TokenType::SEMICOLON);

        // Create wildcard import node
        return std::make_unique<ImportNode>(filePath, ImportType::WILDCARD, loc);
    }

    std::unique_ptr<ASTNode> ImportParser::parseLibraryImport(const SourceLocation& loc)
    {
        // import lib {A, B} from "name";
        if (tokenStream.check(TokenType::LBRACE))
        {
            expectToken(TokenType::LBRACE);

            std::vector<std::string> symbols;
            while (!tokenStream.check(TokenType::RBRACE))
            {
                if (!tokenStream.check(TokenType::IDENTIFIER))
                {
                    throw ParseException("Expected identifier in library import list", tokenStream.current().location);
                }

                std::string symbolName = tokenStream.current().stringValue.getString();
                SourceLocation symbolLocation = tokenStream.current().location;
                ParserUtils::validateIdentifierName(symbolName, "Library import symbol", symbolLocation);
                symbols.push_back(symbolName);
                tokenStream.advance();

                if (tokenStream.check(TokenType::COMMA))
                {
                    tokenStream.advance();
                }
                else if (!tokenStream.check(TokenType::RBRACE))
                {
                    throw ParseException("Expected ',' or '}' in library import list", tokenStream.current().location);
                }
            }

            expectToken(TokenType::RBRACE);

            if (symbols.empty())
            {
                throw ParseException("Library import list cannot be empty", loc);
            }

            expectToken(TokenType::FROM);

            if (!tokenStream.check(TokenType::STRING_LITERAL))
            {
                throw ParseException("Expected library name string after 'from'", tokenStream.current().location);
            }

            std::string libraryName = tokenStream.current().stringValue.getString();
            tokenStream.advance();

            if (libraryName.empty())
            {
                throw ParseException("Library name cannot be empty", tokenStream.current().location);
            }

            expectToken(TokenType::SEMICOLON);

            auto node = std::make_unique<ImportNode>(libraryName, ImportType::LIBRARY_SELECTIVE, symbols, loc);
            node->setLibraryName(libraryName);
            return node;
        }

        // import lib "name";
        if (!tokenStream.check(TokenType::STRING_LITERAL))
        {
            throw ParseException("Expected library name string or '{' after 'lib'", tokenStream.current().location);
        }

        std::string libraryName = tokenStream.current().stringValue.getString();
        tokenStream.advance();

        if (libraryName.empty())
        {
            throw ParseException("Library name cannot be empty", tokenStream.current().location);
        }

        expectToken(TokenType::SEMICOLON);

        auto node = std::make_unique<ImportNode>(libraryName, ImportType::LIBRARY, loc);
        node->setLibraryName(libraryName);
        return node;
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
