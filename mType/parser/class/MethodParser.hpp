#pragma once
#include "../core/IParser.hpp"
#include "../TokenStream.hpp"
#include "../ParseContext.hpp"
#include "../../ast/GenericTypeParameter.hpp"
#include <vector>

namespace parser
{
    class MethodParser : public core::IParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;

    public:
        MethodParser(TokenStream& tokenStream, ParseContext& context);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;
        std::string getParserName() const override;

        // Public methods for coordinated parsing
        std::unique_ptr<ASTNode> parseMethod();
        std::unique_ptr<ASTNode> parseStaticMethod();

    private:
        std::unique_ptr<ASTNode> parseMethodWithModifiers(bool isStatic);
        std::vector<ast::GenericTypeParameter> parseMethodGenericParameters();
        void validateMethodName(const std::string& methodName, bool isStatic);
    };
}