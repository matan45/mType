#pragma once

#include "core/BaseParser.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"
#include "../ast/nodes/classes/InterfaceNode.hpp"
#include <memory>

namespace parser
{
    class InterfaceParser : public core::BaseParser
    {
    public:
        explicit InterfaceParser(TokenStream& stream, ParseContext& ctx);
        ~InterfaceParser();

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        std::unique_ptr<nodes::classes::InterfaceNode> parseInterface();

    private:
        std::vector<GenericTypeParameter> parseGenericTypeParameters();

        void validateInterfaceDeclarationContext();
        void parseInterfaceHeader(
            std::string& interfaceName,
            SourceLocation& location,
            VisibilityModifier& visibility,
            bool& isFinal,
            std::vector<GenericTypeParameter>& genericParams);
        void parseAndValidateExtendsClause(
            nodes::classes::InterfaceNode* interfaceNode,
            const std::string& interfaceName);
        void parseInterfaceBody(nodes::classes::InterfaceNode* interfaceNode);

        // Absorbed from InterfaceMethodSignatureParser
        std::unique_ptr<ASTNode> parseMethodSignature();
    };
}
