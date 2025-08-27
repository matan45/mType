#pragma once
#include <vector>
#include "../ast/ASTNode.hpp"

namespace parser
{
    class Parser;
    using namespace ast;
    
    class NamespaceParser
    {
    private:
        Parser& parser;
        
    public:
        explicit NamespaceParser(Parser& p) : parser(p) {}
        
        // Namespace parsing methods
        std::unique_ptr<ASTNode> parseNamespace();
        std::unique_ptr<ASTNode> parseUsing();
        std::unique_ptr<ASTNode> parseQualifiedName();
        
    private:
        std::vector<std::string> parseNamespacePath();
    };
}

