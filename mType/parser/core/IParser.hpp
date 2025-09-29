#pragma once
#include "../../ast/ASTNode.hpp"
#include "../TokenStream.hpp"
#include <memory>
#include <string>

namespace parser::core
{
    using namespace ast;

    class IParser
    {
    public:
        virtual ~IParser() = default;

        virtual std::unique_ptr<ASTNode> parse() = 0;
        virtual bool canParse(const TokenStream& stream) const = 0;
        virtual std::string getParserName() const = 0;
    };
}