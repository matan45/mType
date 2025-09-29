#pragma once
#include "../core/IParser.hpp"
#include "../TokenStream.hpp"
#include "../ParseContext.hpp"

namespace parser
{
    class ConstructorParser : public core::IParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;

    public:
        ConstructorParser(TokenStream& tokenStream, ParseContext& context);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;
        std::string getParserName() const override;

        // Public method for coordinated parsing
        std::unique_ptr<ASTNode> parseConstructor();
    };
}