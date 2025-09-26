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
        InterfaceParser(TokenStream& stream, ParseContext& ctx) 
            : tokenStream(stream), context(ctx) {}
        
        std::unique_ptr<ast::nodes::classes::InterfaceNode> parseInterface();
        
    private:
        std::unique_ptr<ASTNode> parseMethodSignature();
        std::vector<ast::GenericTypeParameter> parseGenericTypeParameters();
    };
}