#pragma once
#include "../core/BaseParser.hpp"
#include "../../ast/ASTNode.hpp"
#include "../../value/ValueType.hpp"
#include <memory>
#include <vector>

namespace parser::statement
{
    using namespace ast;
    using namespace parser::core;
    using namespace value;

    class FunctionParser : public BaseParser
    {
    public:
        FunctionParser(TokenStream& stream, ParseContext& ctx, std::shared_ptr<error::ErrorHandler> handler)
            : BaseParser(stream, ctx, handler) {}

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;
        std::string getParserName() const override { return "FunctionParser"; }

        std::unique_ptr<ASTNode> parseFunction();
        std::unique_ptr<ASTNode> parseNativeFunction();

    private:
        bool isFunctionToken(token::TokenType type) const noexcept;
        std::vector<std::pair<std::string, ValueType>> parseParameterList();
        void validateFunctionName(const std::string& funcName);
    };
}