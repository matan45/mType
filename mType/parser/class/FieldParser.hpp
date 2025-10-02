#pragma once
#include "../core/IParser.hpp"
#include "../TokenStream.hpp"
#include "../ParseContext.hpp"
#include "../core/BaseParser.hpp"

namespace parser
{
    class FieldParser : public core::BaseParser
    {
    
    public:
        explicit FieldParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        // Public method for coordinated parsing
        std::unique_ptr<ASTNode> parseField();

    private:
        std::pair<bool, bool> parseFieldModifiers(); // Returns {isStatic, isFinal}
        std::unique_ptr<ASTNode> parseFieldDeclaration(bool isStatic, bool isFinal);
        std::unique_ptr<ASTNode> parseInitialValue();
    };
}