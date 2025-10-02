#pragma once
#include "../core/BaseParser.hpp"
#include "../TokenStream.hpp"
#include "../ParseContext.hpp"
#include <vector>
#include <string>
#include <memory>

namespace parser
{
    class GenericParameterParser; // Forward declaration

    class ClassDeclarationParser : public core::BaseParser
    {
    private:
        std::unique_ptr<GenericParameterParser> genericParameterParser;

    public:
        explicit ClassDeclarationParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        // Public methods for coordinate parsing
        std::unique_ptr<ASTNode> parseClassDeclaration();
        std::vector<std::string> parseImplementedInterfaces();
        std::string parseQualifiedClassName();

    private:
        void validateClassName(const std::string& className, const SourceLocation& location);
        std::string parseGenericInterfaceName();
    };
}
