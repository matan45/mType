#pragma once
#include "../core/BaseParser.hpp"
#include "../TokenStream.hpp"
#include "../ParseContext.hpp"
#include "../../ast/ASTNode.hpp"

namespace parser
{
    /// @brief Specialized parser for interface method signatures
    /// Handles parsing of method declarations within interface bodies
    /// Follows Single Responsibility Principle - only parses method signatures (no implementations)
    class InterfaceMethodSignatureParser : public core::BaseParser
    {
    public:
        explicit InterfaceMethodSignatureParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        /// @brief Parse a method signature (declaration only, no implementation)
        /// Used within interface bodies
        /// @return FunctionNode with dummy empty body (interfaces don't have implementations)
        std::unique_ptr<ASTNode> parseMethodSignature();
    };
}
