#pragma once
#include <memory>
#include "../ast/ASTNode.hpp"
#include "../ast/GenericTypeParameter.hpp"
#include "../ast/GenericType.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"

// Forward declarations of helper parsers
namespace parser
{
    class ClassDeclarationParser;
    class ConstructorParser;
    class MethodParser;
    class FieldParser;
    class ObjectCreationParser;
    class GenericParameterParser;
}

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

        // Helper parsers
        std::unique_ptr<ClassDeclarationParser> classDeclarationParser;
        std::unique_ptr<ConstructorParser> constructorParser;
        std::unique_ptr<MethodParser> methodParser;
        std::unique_ptr<FieldParser> fieldParser;
        std::unique_ptr<ObjectCreationParser> objectCreationParser;
        std::unique_ptr<GenericParameterParser> genericParameterParser;

    public:
        explicit ClassParser(TokenStream& stream, ParseContext& ctx);
        ~ClassParser();

        // Class parsing methods - now delegate to helper parsers
        std::unique_ptr<ASTNode> parseClass();
        std::unique_ptr<ASTNode> parseConstructor();
        std::unique_ptr<ASTNode> parseMethod();
        std::unique_ptr<ASTNode> parseField();
        std::unique_ptr<ASTNode> parseNewExpression();

    private:
        void initializeHelperParsers();

        // Delegation methods for backward compatibility
        std::string parseGenericParameters();
        std::string parseGenericParameter();
        std::vector<ast::GenericTypeParameter> parseGenericTypeParameters();
        ast::GenericTypeParameter parseGenericTypeParameter();
    };
}

