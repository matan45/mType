#include "NamespaceParser.hpp"
#include "Parser.hpp"
#include "../ast/nodes/namespaces/NamespaceNode.hpp"
#include "../ast/nodes/namespaces/UsingNode.hpp"
#include "../ast/nodes/namespaces/QualifiedNameNode.hpp"
#include <cctype>

namespace parser
{
    using namespace ast::nodes::namespaces;
    using namespace token;
    
    // Helper function to validate namespace naming convention
    bool isValidNamespaceName(const std::string& name) {
        return !name.empty() && std::islower(name[0]);
    }

    std::unique_ptr<ASTNode> NamespaceParser::parseNamespace()
    {
        parser.expectToken(TokenType::NAMESPACE);
        
        if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected namespace name");
        }
        
        std::string namespaceName = parser.getCurrentToken().stringValue;
        
        // Validate namespace naming convention
        if (!isValidNamespaceName(namespaceName)) {
            throw std::runtime_error("Namespace name '" + namespaceName + "' must start with a lowercase letter");
        }
        
        parser.advanceToken();
        
        parser.expectToken(TokenType::LBRACE);
        
        auto namespaceNode = std::make_unique<NamespaceNode>(namespaceName);
        
        while (parser.getCurrentToken().type != TokenType::RBRACE && parser.getCurrentToken().type != TokenType::END) {
            auto declaration = parser.parseStatement();
            if (declaration) {
                namespaceNode->addDeclaration(std::move(declaration));
            }
        }
        
        parser.expectToken(TokenType::RBRACE);
        
        return std::move(namespaceNode);
    }

    std::unique_ptr<ASTNode> NamespaceParser::parseUsing()
    {
        parser.expectToken(TokenType::USING);
        
        if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected namespace or type name after 'using'");
        }
        
        std::vector<std::string> qualifiedName;
        qualifiedName.push_back(parser.getCurrentToken().stringValue);
        parser.advanceToken();
        
        while (parser.getCurrentToken().type == TokenType::SCOPE) {
            parser.advanceToken();
            
            if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
                throw std::runtime_error("Expected identifier after '::'");
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
                throw std::runtime_error("Expected identifier after '::'");
            }
            
            qualifiedName.push_back(parser.getCurrentToken().stringValue);
            parser.advanceToken();
        }
        
        return std::make_unique<QualifiedNameNode>(qualifiedName);
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
                    throw std::runtime_error("Expected identifier after '::'");
                }
                
                path.push_back(parser.getCurrentToken().stringValue);
                parser.advanceToken();
            }
        }
        
        return path;
    }
}
