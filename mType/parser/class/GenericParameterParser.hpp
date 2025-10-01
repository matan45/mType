#pragma once
#include "../core/IParser.hpp"
#include "../TokenStream.hpp"
#include "../ParseContext.hpp"
#include "../../ast/GenericTypeParameter.hpp"
#include <vector>
#include <string>

namespace parser
{
    class GenericParameterParser : public core::IParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;

    public:
        GenericParameterParser(TokenStream& tokenStream, ParseContext& context);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;
        std::string getParserName() const override;

        // Public methods for coordinated parsing
        std::string parseGenericParameters();
        std::string parseGenericParameter();
        std::vector<ast::GenericTypeParameter> parseGenericTypeParameters();
        ast::GenericTypeParameter parseGenericTypeParameter();

    private:
        std::string parseNestedGenericType();
        std::string parseNestedGenericConstraint();
        void validateGenericParameterName(const std::string& paramName);
    };
}