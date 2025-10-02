#pragma once
#include "../core/IParser.hpp"
#include "../TokenStream.hpp"
#include "../ParseContext.hpp"
#include "../TypeParser.hpp"
#include "../core/BaseParser.hpp"

namespace parser
{
    class ObjectCreationParser : public core::BaseParser
    {
    
    public:
        explicit ObjectCreationParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

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