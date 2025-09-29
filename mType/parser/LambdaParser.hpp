#pragma once
#include "../ast/nodes/expressions/LambdaNode.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"
#include <memory>
#include <vector>

namespace parser
{
    using namespace ast::nodes::expressions;

    class LambdaParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;

    public:
        explicit LambdaParser(TokenStream& stream, ParseContext& ctx)
            : tokenStream(stream), context(ctx) {}

        // Main lambda parsing entry point
        std::unique_ptr<LambdaNode> parseLambda();

        // Check if current position looks like a lambda
        bool isLambdaStart() const;

    private:
        // Parse lambda parameter list
        std::vector<Parameter> parseLambdaParameters();

        // Parse single parameter with optional type annotation
        Parameter parseParameter();

        // Parse lambda body (expression or block)
        std::pair<std::unique_ptr<ASTNode>, BodyType> parseLambdaBody();

        // Helper methods
        bool isLambdaParameterPattern() const;
        bool hasArrowAhead() const;
    };
}
