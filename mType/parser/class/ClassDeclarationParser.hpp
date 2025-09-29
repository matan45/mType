#pragma once
#include "../core/IParser.hpp"
#include "../TokenStream.hpp"
#include "../ParseContext.hpp"
#include "../../ast/GenericTypeParameter.hpp"
#include <vector>
#include <string>

namespace parser
{
    class ClassDeclarationParser : public core::IParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;

    public:
        ClassDeclarationParser(TokenStream& tokenStream, ParseContext& context);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;
        std::string getParserName() const override;

        // Public methods for coordinate parsing
        std::unique_ptr<ASTNode> parseClassDeclaration();
        std::vector<std::string> parseImplementedInterfaces();
        std::string parseQualifiedClassName();

    private:
        void validateClassName(const std::string& className, const SourceLocation& location);
        std::string parseGenericInterfaceName();
    };
}