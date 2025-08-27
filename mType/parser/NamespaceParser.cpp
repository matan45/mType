#include "NamespaceParser.hpp"
#include "Parser.hpp"
#include "../ast/nodes/namespaces/NamespaceNode.hpp"

namespace parser
{
    using namespace ast::nodes::namespaces;
    using namespace token;

    std::unique_ptr<ASTNode> NamespaceParser::parseNamespace()
    {
        parser.expectToken(TokenType::NAMESPACE);
        
        if (parser.getCurrentToken().type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected namespace name");
        }
        
        std::string namespaceName = parser.getCurrentToken().stringValue;
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
        return nullptr;
    }

    std::unique_ptr<ASTNode> NamespaceParser::parseQualifiedName()
    {
        return nullptr;
    }

    std::vector<std::string> NamespaceParser::parseNamespacePath()
    {
        return std::vector<std::string>();
    }
}
