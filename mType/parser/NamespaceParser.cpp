#include "NamespaceParser.hpp"
#include "Parser.hpp"
#include "../ast/nodes/namespaces/NamespaceNode.hpp"
#include "../ast/nodes/namespaces/UsingNode.hpp"
#include "../ast/nodes/namespaces/QualifiedNameNode.hpp"
#include "../errors/ParseException.hpp"
#include "ParserValidator.hpp"

namespace parser
{
    using namespace ast::nodes::namespaces;
    using namespace token;
    
    

    std::unique_ptr<ASTNode> NamespaceParser::parseNamespace()
    {
        parser.expectToken(TokenType::NAMESPACE);
        
        if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
            throw ParseException("Expected namespace name", parser.getCurrentToken().location);
        }
        
        // Parse qualified namespace name (e.g., a::b::c)
        std::vector<std::string> nameParts;
        nameParts.push_back(parser.getCurrentToken().stringValue);
        
        // Validate first part naming convention
        if (!ParserValidator::isValidNamespaceName(nameParts[0])) {
            throw ParseException("Namespace name '" + nameParts[0] + "' must start with a lowercase letter", parser.getCurrentToken().location);
        }
        
        parser.advanceToken();
        
        // Parse additional parts separated by ::
        while (parser.getCurrentToken().type == TokenType::SCOPE) {
            parser.advanceToken();
            
            if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
                throw ParseException("Expected identifier after '::'", parser.getCurrentToken().location);
            }
            
            std::string nextPart = parser.getCurrentToken().stringValue;
            
            // Validate each part naming convention
            if (!ParserValidator::isValidNamespaceName(nextPart)) {
                throw ParseException("Namespace name '" + nextPart + "' must start with a lowercase letter", parser.getCurrentToken().location);
            }
            
            nameParts.push_back(nextPart);
            parser.advanceToken();
        }
        
        parser.expectToken(TokenType::LBRACE);
        
        // Parse namespace body
        std::vector<std::unique_ptr<ASTNode>> declarations;
        while (parser.getCurrentToken().type != TokenType::RBRACE && parser.getCurrentToken().type != TokenType::END) {
            auto declaration = parser.parseStatement();
            if (declaration) {
                declarations.push_back(std::move(declaration));
            }
        }
        
        parser.expectToken(TokenType::RBRACE);
        
        // Create nested namespace structure
        // For "namespace a::b::c {}", create: a { b { c { declarations } } }
        return createNestedNamespace(nameParts, std::move(declarations));
    }

    std::unique_ptr<ASTNode> NamespaceParser::parseUsing()
    {
        parser.expectToken(TokenType::USING);
        
        if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
            throw ParseException("Expected namespace or type name after 'using'", parser.getCurrentToken().location);
        }
        
        std::vector<std::string> qualifiedName;
        qualifiedName.push_back(parser.getCurrentToken().stringValue);
        parser.advanceToken();
        
        while (parser.getCurrentToken().type == TokenType::SCOPE) {
            parser.advanceToken();
            
            if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
                throw ParseException("Expected identifier after '::'", parser.getCurrentToken().location);
            }
            
            qualifiedName.push_back(parser.getCurrentToken().stringValue);
            parser.advanceToken();
        }
        
        parser.expectToken(TokenType::SEMICOLON);
        
        return std::make_unique<UsingNode>(qualifiedName);
    }

    std::unique_ptr<ASTNode> NamespaceParser::parseQualifiedName()
    {
        if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
            return nullptr;
        }
        
        std::vector<std::string> qualifiedName;
        qualifiedName.push_back(parser.getCurrentToken().stringValue);
        parser.advanceToken();
        
        while (parser.getCurrentToken().type == TokenType::SCOPE) {
            parser.advanceToken();
            
            if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
                throw ParseException("Expected identifier after '::'", parser.getCurrentToken().location);
            }
            
            qualifiedName.push_back(parser.getCurrentToken().stringValue);
            parser.advanceToken();
        }
        
        return std::make_unique<QualifiedNameNode>(qualifiedName);
    }

    std::unique_ptr<ASTNode> NamespaceParser::createNestedNamespace(
        const std::vector<std::string>& nameParts,
        std::vector<std::unique_ptr<ASTNode>> declarations)
    {
        if (nameParts.empty()) {
            return nullptr;
        }

        // Start from the innermost namespace and work outward
        // For "a::b::c", we create: c { declarations }, then b { c {...} }, then a { b {...} }
        
        // Create the innermost namespace with the actual declarations
        auto innermost = std::make_unique<NamespaceNode>(nameParts.back(), std::move(declarations));
        
        // If there's only one namespace part, return it directly
        if (nameParts.size() == 1) {
            return std::move(innermost);
        }
        
        // Wrap each outer namespace around the inner ones
        std::unique_ptr<ASTNode> current = std::move(innermost);
        
        // Work backwards through the namespace parts (excluding the last one we already handled)
        for (int i = static_cast<int>(nameParts.size()) - 2; i >= 0; --i) {
            std::vector<std::unique_ptr<ASTNode>> wrapperDeclarations;
            wrapperDeclarations.push_back(std::move(current));
            
            current = std::make_unique<NamespaceNode>(nameParts[i], std::move(wrapperDeclarations));
        }
        
        return current;
    }

    std::vector<std::string> NamespaceParser::parseNamespacePath()
    {
        std::vector<std::string> path;
        
        if (parser.getCurrentToken().type == TokenType::IDENTIFIER) {
            path.push_back(parser.getCurrentToken().stringValue);
            parser.advanceToken();
            
            while (parser.getCurrentToken().type == TokenType::SCOPE) {
                parser.advanceToken();
                
                if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
                    throw ParseException("Expected identifier after '::'", parser.getCurrentToken().location);
                }
                
                path.push_back(parser.getCurrentToken().stringValue);
                parser.advanceToken();
            }
        }
        
        return path;
    }
}
