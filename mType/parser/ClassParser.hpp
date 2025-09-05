#pragma once
#include <memory>
#include "../ast/ASTNode.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"

namespace parser
{
    class ParseContext;
    using namespace ast;
    class ClassParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;
        
    public:
        explicit ClassParser(TokenStream& stream, ParseContext& ctx) : tokenStream(stream), context(ctx) {}
        
        // Class parsing methods
        std::unique_ptr<ASTNode> parseClass();
        std::unique_ptr<ASTNode> parseConstructor();
        std::unique_ptr<ASTNode> parseMethod();
        std::unique_ptr<ASTNode> parseField();
        std::unique_ptr<ASTNode> parseNewExpression();
        
    private:
    
    };
}

