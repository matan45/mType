#pragma once
#include <memory>
#include "../ast/ASTNode.hpp"
#include "../ast/GenericTypeParameter.hpp"
#include "../ast/GenericType.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"

namespace parser
{
    class ParseContext;
    struct TypeInfo;  // Forward declaration
    using namespace ast;
    class ClassParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;
        
    public:
        explicit ClassParser(TokenStream& stream, ParseContext& ctx) : tokenStream(stream), context(ctx) {}
        
        // Class parsing methods
        std::unique_ptr<ASTNode> parseClass();
        std::unique_ptr<ASTNode> parseConstructor();
        std::unique_ptr<ASTNode> parseMethod();
        std::unique_ptr<ASTNode> parseField();
        std::unique_ptr<ASTNode> parseNewExpression();
        
    private:
        // Helper methods for parsing nested generic parameters (for instantiation)
        std::string parseGenericParameters();
        std::string parseGenericParameter();

        // NEW: Helper methods for parsing generic type parameter declarations
        std::vector<ast::GenericTypeParameter> parseGenericTypeParameters();
        ast::GenericTypeParameter parseGenericTypeParameter();

        // NEW: Helper method to convert TypeInfo to GenericType
        std::shared_ptr<ast::GenericType> convertTypeInfoToGenericType(const TypeInfo& typeInfo);
    };
}

