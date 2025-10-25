#pragma once
#include "../core/IParser.hpp"
#include "../TokenStream.hpp"
#include "../ParseContext.hpp"
#include "../../ast/GenericTypeParameter.hpp"
#include "../core/BaseParser.hpp"
#include <vector>
#include <string>
#include <unordered_set>

namespace parser
{
    class GenericParameterParser : public core::BaseParser
    {
    public:
        explicit GenericParameterParser(TokenStream& stream, ParseContext& ctx);

        std::unique_ptr<ASTNode> parse() override;
        bool canParse(const TokenStream& stream) const override;

        // Public methods for coordinated parsing
        std::string parseGenericParameters();
        std::string parseGenericParameter();
        std::vector<GenericTypeParameter> parseGenericTypeParameters();
        GenericTypeParameter parseGenericTypeParameter();

    private:
        std::string parseNestedGenericConstraint();
        void validateGenericParameterName(const std::string& paramName);
        void validateNoCircularDependencies(const std::vector<GenericTypeParameter>& parameters);
        bool hasCircularDependency(
            const std::string& paramName,
            const std::vector<GenericTypeParameter>& parameters,
            std::unordered_set<std::string>& visited,
            std::unordered_set<std::string>& recursionStack);
        static std::vector<std::string> extractTypeParameters(
            const std::string& constraint,
            const std::vector<GenericTypeParameter>& parameters);
    };
}
