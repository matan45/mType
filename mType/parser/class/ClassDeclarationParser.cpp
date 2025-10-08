#include "ClassDeclarationParser.hpp"
#include "GenericParameterParser.hpp"
#include "../ParserValidator.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../errors/ParseException.hpp"

namespace parser
{
    using namespace ast::nodes::classes;
    using namespace token;
    using namespace errors;

    ClassDeclarationParser::ClassDeclarationParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
        genericParameterParser = std::make_unique<GenericParameterParser>(tokenStream, context);
    }

    std::unique_ptr<ASTNode> ClassDeclarationParser::parse()
    {
        return parseClassDeclaration();
    }

    bool ClassDeclarationParser::canParse(const TokenStream& stream) const
    {
        return stream.check(TokenType::CLASS);
    }

    std::unique_ptr<ASTNode> ClassDeclarationParser::parseClassDeclaration()
    {
        // Check for optional 'final' keyword
        bool isFinal = false;
        if (tokenStream.check(TokenType::FINAL))
        {
            isFinal = true;
            tokenStream.advance(); // consume 'final'
        }

        tokenStream.expect(TokenType::CLASS);

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected class name", tokenStream.current().location);
        }

        std::string className = tokenStream.current().stringValue.getString();
        validateClassName(className, tokenStream.current().location);
        tokenStream.advance();

        // Parse generic type parameters if present
        std::vector<ast::GenericTypeParameter> genericParameters;
        if (tokenStream.check(TokenType::LESS))
        {
            tokenStream.advance(); // consume '<'
            genericParameters = genericParameterParser->parseGenericTypeParameters();
            tokenStream.expect(TokenType::GREATER); // consume '>'
        }

        // Parse extends clause if present (must come before implements)
        std::string parentClassName = parseExtendsClause();

        // Parse implements clause if present
        std::vector<std::string> implementedInterfaces = parseImplementedInterfaces();

        tokenStream.expect(TokenType::LBRACE);

        auto classNode = std::make_unique<ClassNode>(className, genericParameters, parentClassName, implementedInterfaces);
        classNode->setFinal(isFinal);
        return classNode;
    }

    std::string ClassDeclarationParser::parseExtendsClause()
    {
        if (!tokenStream.check(TokenType::EXTENDS))
        {
            return ""; // No inheritance
        }

        tokenStream.advance(); // consume 'extends'

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected parent class name after 'extends'",
                               tokenStream.current().location);
        }

        std::string parentClass = parseGenericInterfaceName(); // Reuse for generic parent support

        // Validate parent class name
        ParserUtils::validateCapitalizedName(parentClass.substr(0, parentClass.find('<')),
                                            "Parent class",
                                            tokenStream.current().location);

        return parentClass;
    }

    std::vector<std::string> ClassDeclarationParser::parseImplementedInterfaces()
    {
        std::vector<std::string> implementedInterfaces;

        if (tokenStream.check(TokenType::IMPLEMENTS))
        {
            tokenStream.advance(); // consume 'implements'

            // Use ParserUtils to parse the interface list
            implementedInterfaces = ParserUtils::parseInterfaceList(tokenStream, "implements");
        }

        return implementedInterfaces;
    }

    std::string ClassDeclarationParser::parseQualifiedClassName()
    {
        std::vector<std::string> qualifiedParts;
        qualifiedParts.push_back(tokenStream.current().stringValue.getString());
        tokenStream.advance();

        while (tokenStream.current().type == TokenType::SCOPE)
        {
            tokenStream.advance();
            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected identifier after '::'", tokenStream.current().location);
            }
            qualifiedParts.push_back(tokenStream.current().stringValue.getString());
            tokenStream.advance();
        }

        // Reconstruct full qualified name
        std::string className = qualifiedParts[0];
        for (size_t i = 1; i < qualifiedParts.size(); ++i)
        {
            className += "::" + qualifiedParts[i];
        }

        return className;
    }

    void ClassDeclarationParser::validateClassName(const std::string& className, const SourceLocation& location)
    {
        // Use ParserUtils for consistent validation
        ParserUtils::validateCapitalizedName(className, "Class", location);
    }

    std::string ClassDeclarationParser::parseGenericInterfaceName()
    {
        std::string interfaceName = tokenStream.current().stringValue.getString();
        tokenStream.advance();

        // Handle generic parameters for interface if present
        if (tokenStream.check(TokenType::LESS))
        {
            interfaceName += "<";
            tokenStream.advance(); // consume '<'

            int depth = 1;
            while (depth > 0 && !tokenStream.isAtEnd())
            {
                if (tokenStream.current().type == TokenType::LESS)
                {
                    depth++;
                    interfaceName += "<";
                }
                else if (tokenStream.current().type == TokenType::GREATER)
                {
                    depth--;
                    interfaceName += ">";
                }
                else if (tokenStream.current().type == TokenType::IDENTIFIER)
                {
                    interfaceName += tokenStream.current().stringValue.getString();
                }
                else if (tokenStream.current().type == TokenType::STRING_TYPE)
                {
                    interfaceName += "string";
                }
                else if (tokenStream.current().type == TokenType::INT)
                {
                    interfaceName += "int";
                }
                else if (tokenStream.current().type == TokenType::FLOAT)
                {
                    interfaceName += "float";
                }
                else if (tokenStream.current().type == TokenType::BOOL)
                {
                    interfaceName += "bool";
                }
                else if (tokenStream.current().type == TokenType::VOID)
                {
                    interfaceName += "void";
                }
                else if (tokenStream.current().type == TokenType::COMMA)
                {
                    interfaceName += ", ";
                }

                tokenStream.advance();
            }
        }

        return interfaceName;
    }
}
