#pragma once
#include "../core/BaseParser.hpp"
#include "../../ast/ASTNode.hpp"
#include <memory>
#include <vector>
#include <string>

namespace parser::expression
{
    using namespace ast;
    using namespace parser::core;

    class ArgumentParser : public BaseParser
    {
    public:
        ArgumentParser(TokenStream& stream, ParseContext& ctx, std::shared_ptr<error::ErrorHandler> handler)
            : BaseParser(stream, ctx, handler) {}

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;
        std::string getParserName() const override { return "ArgumentParser"; }

        std::vector<std::unique_ptr<ASTNode>> parseArguments();
        std::vector<std::string> parseGenericTypeArguments();

    private:
        std::string parseGenericTypeArgument();
        std::string parseNestedGenericType();
    };
}