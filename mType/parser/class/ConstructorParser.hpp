#pragma once
#include "../core/IParser.hpp"
#include "../TokenStream.hpp"
#include "../ParseContext.hpp"
#include "../core/BaseParser.hpp"

namespace parser
{
    class ConstructorParser : public core::BaseParser
    {
    
    public:
        ConstructorParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        // Public method for coordinated parsing
        std::unique_ptr<ASTNode> parseConstructor();
    };
}