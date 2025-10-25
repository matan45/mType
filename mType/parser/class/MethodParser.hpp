#pragma once
#include "../core/IParser.hpp"
#include "../TokenStream.hpp"
#include "../ParseContext.hpp"
#include "../../ast/GenericTypeParameter.hpp"
#include "../../ast/AccessModifier.hpp"
#include "../core/BaseParser.hpp"
#include <vector>

namespace parser
{
    class MethodParser : public core::BaseParser
    {
    public:
        explicit MethodParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        // Public methods for coordinated parsing
        std::unique_ptr<ASTNode> parseMethod();
        std::unique_ptr<ASTNode> parseStaticMethod();

    private:
        std::unique_ptr<ASTNode> parseMethodWithModifiers(ast::AccessModifier accessModifier, bool isStatic,
                                                          bool isAsync = false, bool isAbstract = false,
                                                          const SourceLocation& loc = SourceLocation());
        std::vector<GenericTypeParameter> parseMethodGenericParameters();
        void validateMethodName(const std::string& methodName, bool isStatic);
    };
}
