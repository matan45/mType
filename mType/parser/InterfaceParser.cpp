#include "InterfaceParser.hpp"
#include "ParserConstants.hpp"
#include "ParserContextState.hpp"
#include "class/GenericParameterParser.hpp"
#include "interface/InterfaceMethodSignatureParser.hpp"
#include "TypeParser.hpp"
#include "utilities/ParserUtils.hpp"
#include "utilities/NameValidator.hpp"
#include "utilities/AccessModifierParser.hpp"
#include "utilities/VisibilityParser.hpp"
#include "../token/TokenType.hpp"
#include "../ast/nodes/classes/InterfaceNode.hpp"
#include "../errors/ParseException.hpp"
#include "../errors/DuplicateDeclarationException.hpp"

namespace parser
{
    using namespace ast::nodes::classes;
    using namespace token;
    using namespace errors;
    using namespace parser::utilities;

    InterfaceParser::InterfaceParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
        initializeHelperParsers();
    }

    InterfaceParser::~InterfaceParser() = default;

    void InterfaceParser::initializeHelperParsers()
    {
        methodSignatureParser = std::make_unique<InterfaceMethodSignatureParser>(tokenStream, context);
    }

    std::unique_ptr<ASTNode> InterfaceParser::parse()
    {
        return parseInterface();
    }

    bool InterfaceParser::canParse(const TokenStream& stream) const
    {
        TokenType current = stream.current().type;
        // Check for optional visibility modifiers followed by optional 'final' and 'interface'
        return current == TokenType::INTERFACE ||
            current == TokenType::PUBLIC ||
            current == TokenType::PRIVATE ||
            current == TokenType::FINAL;
    }

    std::unique_ptr<InterfaceNode> InterfaceParser::parseInterface()
    {
        // Step 1: Validate declaration context
        validateInterfaceDeclarationContext();

        // Step 2: Parse interface header
        std::string interfaceName;
        SourceLocation location;
        VisibilityModifier visibility;
        bool isFinal = false;
        std::vector<GenericTypeParameter> genericParams;

        parseInterfaceHeader(interfaceName, location, visibility, isFinal, genericParams);

        // Step 3: Create interface node and register it
        auto interfaceNode = std::make_unique<InterfaceNode>(
            interfaceName, genericParams, tokenStream.current().location
        );

        // Check for duplicate class/interface name
        if (context.isTypeDeclared(interfaceName))
        {
            // Get the location of the first declaration for better error message
            SourceLocation firstLocation = context.getTypeDeclarationLocation(interfaceName);
            throw DuplicateDeclarationException(
                "interface",
                interfaceName,
                firstLocation,
                location
            );
        }

        // Register the interface name with final modifier and location
        context.registerInterface(interfaceName, isFinal, location);

        // Step 4: Parse and validate extends clause
        parseAndValidateExtendsClause(interfaceNode.get(), interfaceName);

        // Step 5: Parse interface body
        parseInterfaceBody(interfaceNode.get());

        // Step 6: Set final attributes and return
        interfaceNode->setFinal(isFinal);
        interfaceNode->setVisibility(visibility);
        return interfaceNode;
    }

    void InterfaceParser::validateInterfaceDeclarationContext()
    {
        // Validate: interfaces cannot be declared inside classes or other interfaces
        if (context.isInsideClassBody())
        {
            throw ParseException("Interface declarations inside class bodies are not allowed.",
                                 tokenStream.current().location);
        }
        if (context.isInsideInterfaceBody())
        {
            throw ParseException("Interface declarations inside interface bodies are not allowed. "
                                 "Nested interfaces are not supported.",
                                 tokenStream.current().location);
        }
    }

    void InterfaceParser::parseInterfaceHeader(
        std::string& interfaceName,
        SourceLocation& location,
        VisibilityModifier& visibility,
        bool& isFinal,
        std::vector<GenericTypeParameter>& genericParams)
    {
        // Parse optional visibility modifier (public/private)
        visibility = VisibilityParser::parseVisibilityModifier(tokenStream);

        // Check for optional 'final' keyword
        isFinal = false;
        if (tokenStream.check(TokenType::FINAL))
        {
            isFinal = true;
            tokenStream.advance(); // consume 'final'
        }

        // Consume 'interface' token
        if (tokenStream.current().type != TokenType::INTERFACE)
        {
            throw ParseException("Expected 'interface' keyword", tokenStream.current().location);
        }
        tokenStream.advance();

        // Parse interface name
        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected interface name", tokenStream.current().location);
        }

        interfaceName = tokenStream.current().stringValue.getString();
        location = tokenStream.current().location;

        // Validate interface name using NameValidator
        NameValidator::validateCapitalizedName(interfaceName, "Interface", location);

        tokenStream.advance();

        // Parse optional generic type parameters
        if (tokenStream.current().type == TokenType::LESS)
        {
            tokenStream.advance(); // consume '<'
            genericParams = parseGenericTypeParameters();
            tokenStream.expect(TokenType::GREATER); // consume '>'

            // Validate generic parameter count limit
            if (genericParams.size() > constants::MAX_GENERIC_PARAMETERS)
            {
                throw ParseException(
                    "Interface '" + interfaceName + "' has too many generic parameters (" +
                    std::to_string(genericParams.size()) + "). Maximum allowed: " +
                    std::to_string(constants::MAX_GENERIC_PARAMETERS),
                    location);
            }
        }
    }

    void InterfaceParser::parseAndValidateExtendsClause(
        nodes::classes::InterfaceNode* interfaceNode,
        const std::string& interfaceName)
    {
        if (tokenStream.current().type != TokenType::EXTENDS)
        {
            return; // No extends clause
        }

        tokenStream.advance(); // consume 'extends'

        // Use ParserUtils to parse the interface list
        auto extendedInterfaces = ParserUtils::parseInterfaceList(tokenStream, "extends");

        // Validate each parent interface
        for (const auto& parentInterfaceName : extendedInterfaces)
        {
            // Extract base name without generic parameters for validation
            std::string baseParentName = parentInterfaceName;
            size_t genericStart = parentInterfaceName.find('<');
            if (genericStart != std::string::npos)
            {
                baseParentName = parentInterfaceName.substr(0, genericStart);
            }

            // Check if parent is a declared class
            if (context.isClassDeclared(baseParentName))
            {
                throw ParseException(
                    "Interface '" + interfaceName + "' cannot extend class '" + baseParentName + "'. "
                    "Interfaces can only extend other interfaces.",
                    tokenStream.current().location);
            }

            // Check if parent interface is marked as final
            if (context.isInterfaceFinal(baseParentName))
            {
                throw ParseException(
                    "Interface '" + interfaceName + "' cannot extend final interface '" + baseParentName + "'.",
                    tokenStream.current().location);
            }

            interfaceNode->addExtendedInterface(parentInterfaceName);
        }

        // Check for circular inheritance after validating all parents
        if (!context.registerInterfaceInheritance(interfaceName, extendedInterfaces))
        {
            // Build a simple error message
            std::string parentsStr;
            for (size_t i = 0; i < extendedInterfaces.size(); ++i)
            {
                if (i > 0) parentsStr += ", ";
                parentsStr += extendedInterfaces[i];
            }

            throw ParseException(
                "Circular inheritance detected: Interface '" + interfaceName +
                "' creates a cycle when extending [" + parentsStr + "]",
                tokenStream.current().location);
        }
    }

    void InterfaceParser::parseInterfaceBody(nodes::classes::InterfaceNode* interfaceNode)
    {
        // Expect opening brace
        if (tokenStream.current().type != TokenType::LBRACE)
        {
            throw ParseException("Expected '{' to start interface body",
                                 tokenStream.current().location);
        }
        tokenStream.advance();

        // Set interface context when parsing interface body
        ParserContextState::InterfaceContextGuard interfaceGuard(context.getContextState());

        // Parse method signatures
        while (tokenStream.current().type != TokenType::RBRACE && !tokenStream.isAtEnd())
        {
            // Check for access modifiers - validate they are PUBLIC or not present
            if (utilities::AccessModifierParser::isAccessModifier(tokenStream.current().type))
            {
                auto location = tokenStream.current().location;
                ast::AccessModifier modifier = utilities::AccessModifierParser::tokenTypeToAccessModifier(
                    tokenStream.current().type);

                // Validate that interface methods can only be public
                utilities::AccessModifierParser::validateModifierForContext(modifier, true, location);

                tokenStream.advance(); // consume the access modifier
            }

            if (tokenStream.current().type == TokenType::FUNCTION)
            {
                auto methodSignature = methodSignatureParser->parseMethodSignature();
                if (methodSignature)
                {
                    interfaceNode->addMethod(std::move(methodSignature));
                }
            }
            else
            {
                throw ParseException("Only function signatures allowed in interfaces",
                                     tokenStream.current().location);
            }
        }

        // Expect closing brace
        if (tokenStream.current().type != TokenType::RBRACE)
        {
            throw ParseException("Expected '}' to end interface body",
                                 tokenStream.current().location);
        }
        tokenStream.advance();
    }

    std::vector<GenericTypeParameter> InterfaceParser::parseGenericTypeParameters()
    {
        // Use GenericParameterParser to handle all the complexity
        GenericParameterParser genericParser(tokenStream, context);
        return genericParser.parseGenericTypeParameters();
    }
}
