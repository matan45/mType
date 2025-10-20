#pragma once

#include "core/BaseParser.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"
#include "../ast/nodes/classes/InterfaceNode.hpp"
#include <memory>

// Forward declarations of helper parsers
namespace parser
{
    class InterfaceMethodSignatureParser;
}

namespace parser
{
    /// @brief Parser for interface declarations
    /// Now inherits from BaseParser for consistency with ClassParser and FieldParser
    /// Delegates method signature parsing to InterfaceMethodSignatureParser
    class InterfaceParser : public core::BaseParser
    {
    private:
        // Helper parser
        std::unique_ptr<InterfaceMethodSignatureParser> methodSignatureParser;

    public:
        explicit InterfaceParser(TokenStream& stream, ParseContext& ctx);
        ~InterfaceParser();

        // IParser interface implementation
        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        /// @brief Parse a complete interface declaration
        std::unique_ptr<nodes::classes::InterfaceNode> parseInterface();

    private:
        std::vector<GenericTypeParameter> parseGenericTypeParameters();

        // Helper methods for parseInterface refactoring
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

        // Helper parser initialization
        void initializeHelperParsers();
    };
}
