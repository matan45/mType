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
        explicit FunctionParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        std::unique_ptr<ASTNode> parseFunction();

    private:
        bool isFunctionToken(TokenType type) const noexcept;
        std::vector<std::pair<std::string, ValueType>> parseParameterList();
        void validateFunctionName(const std::string& funcName);
    };
}
