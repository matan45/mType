#include "ClassDeclarationParser.hpp"
#include "GenericParameterParser.hpp"
#include "../ParserConstants.hpp"
#include "../ParserValidator.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../utilities/VisibilityParser.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../errors/ParseException.hpp"

namespace parser
{
    using namespace ast::nodes::classes;
    using namespace token;
    using namespace errors;
    using namespace parser::utilities;

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
        // Parse optional visibility modifier (public/private)
        // Default is PUBLIC if not specified
        VisibilityModifier visibility = VisibilityParser::parseVisibilityModifier(tokenStream);

        // Check for optional 'abstract' keyword
        bool isAbstract = false;
        if (tokenStream.check(TokenType::ABSTRACT))
        {
            isAbstract = true;
            tokenStream.advance(); // consume 'abstract'
        }

        // Check for optional 'final' keyword
        bool isFinal = false;
        if (tokenStream.check(TokenType::FINAL))
        {
            isFinal = true;
            tokenStream.advance(); // consume 'final'
        }

        // Validate that abstract and final are mutually exclusive
        if (isAbstract && isFinal)
        {
            throw ParseException(
                "Class cannot be both abstract and final. Abstract classes are meant to be extended, "
                "while final classes cannot be extended.",
                tokenStream.current().location);
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

            // Validate generic parameter count limit
            if (genericParameters.size() > constants::MAX_GENERIC_PARAMETERS)
            {
                throw ParseException(
                    "Class '" + className + "' has too many generic parameters (" +
                    std::to_string(genericParameters.size()) + "). Maximum allowed: " +
                    std::to_string(constants::MAX_GENERIC_PARAMETERS),
                    tokenStream.current().location);
            }
        }

        // Parse extends clause if present (must come before implements)
        std::string parentClassName = parseExtendsClause();

        // Validate that parent is not an interface (if extends clause present)
        if (!parentClassName.empty())
        {
            // Extract base name without generic parameters for validation
            std::string baseParentName = parentClassName;
            size_t genericStart = parentClassName.find('<');
            if (genericStart != std::string::npos)
            {
                baseParentName = parentClassName.substr(0, genericStart);
            }

            // Check if parent is a declared interface
            if (context.isInterfaceDeclared(baseParentName))
            {
                throw ParseException(
                    "Class '" + className + "' cannot extend interface '" + baseParentName + "'. "
                    "Classes can only extend other classes. Use 'implements' for interfaces.",
                    tokenStream.current().location);
            }

            // Check if parent class is marked as final
            if (context.isClassFinal(baseParentName))
            {
                throw ParseException(
                    "Class '" + className + "' cannot extend final class '" + baseParentName + "'.",
                    tokenStream.current().location);
            }

            // Check for circular inheritance
            if (!context.registerClassInheritance(className, baseParentName))
            {
                // Build inheritance chain for error message
                auto chain = context.getClassInheritanceChain(className);
                std::string chainStr = className;
                for (const auto& ancestor : chain)
                {
                    if (ancestor != className)
                    {
                        chainStr += " -> " + ancestor;
                    }
                }
                chainStr += " -> " + baseParentName + " (creates cycle)";

                throw ParseException(
                    "Circular inheritance detected: " + chainStr,
                    tokenStream.current().location);
            }
        }

        // Parse implements clause if present
        std::vector<std::string> implementedInterfaces = parseImplementedInterfaces();

        tokenStream.expect(TokenType::LBRACE);

        auto classNode = std::make_unique<ClassNode>(className, genericParameters, parentClassName,
                                                     implementedInterfaces);
        classNode->setFinal(isFinal);
        classNode->setAbstract(isAbstract);
        classNode->setVisibility(visibility);
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
            interfaceName += ParserUtils::parseNestedGenericExpression(tokenStream);
        }

        return interfaceName;
    }
}
