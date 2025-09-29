#pragma once
#include "../core/IParser.hpp"
#include "../TokenStream.hpp"
#include "../ParseContext.hpp"
#include "../TypeParser.hpp"

namespace parser
{
    class ObjectCreationParser : public core::IParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;

    public:
        ObjectCreationParser(TokenStream& tokenStream, ParseContext& context);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;
        std::string getParserName() const override;

        // Public method for coordinated parsing
        std::unique_ptr<ASTNode> parseNewExpression();

    private:
        std::unique_ptr<ASTNode> parseArrayCreation(const std::string& className);
        std::unique_ptr<ASTNode> parseObjectInstantiation(const std::string& className);
        std::string parseClassNameWithGenerics();
        std::vector<std::unique_ptr<ASTNode>> parseConstructorArguments();
        std::vector<std::unique_ptr<ASTNode>> parseArrayDimensions();
        TypeInfo createElementTypeInfo(const std::string& className);
        void validateClassNameForInstantiation(const std::string& finalClassName);
    };
}