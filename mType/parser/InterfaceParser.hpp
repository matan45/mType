#pragma once

#include "TokenStream.hpp"
#include "ParseContext.hpp"
#include "../ast/nodes/classes/InterfaceNode.hpp"

namespace parser {
    class InterfaceParser {
    private:
        TokenStream& tokenStream;
        ParseContext& context;
        
    public:
        explicit InterfaceParser(TokenStream& stream, ParseContext& ctx); 
        
        std::unique_ptr<nodes::classes::InterfaceNode> parseInterface();
        
    private:
        std::unique_ptr<ASTNode> parseMethodSignature();
        std::vector<GenericTypeParameter> parseGenericTypeParameters();
    };
}