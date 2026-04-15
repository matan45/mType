#include "ParameterParser.hpp"
#include "NameValidator.hpp"
#include "AnnotationParser.hpp"
#include "../TypeParser.hpp"
#include "../TokenStream.hpp"
#include "../../ast/GenericType.hpp"
#include "../../errors/ParseException.hpp"
#include "../../errors/SourceLocation.hpp"
#include "../../token/TokenType.hpp"

namespace parser
{
    // --- MYT-110: annotation-aware parameter parsers ------------------------
    // These produce *Declaration structs carrying annotations. The legacy
    // pair-returning methods below delegate here and discard annotations so
    // existing call sites that don't care about annotations keep compiling.

    std::vector<ValueParameterDeclaration>
    ParameterParser::parseParameterDeclList(TokenStream& stream, bool expectParentheses)
    {
        using namespace value;
        using namespace errors;
        using namespace token;

        std::vector<ValueParameterDeclaration> parameters;

        if (expectParentheses) {
            stream.expect(TokenType::LPAREN);
        }

        if (stream.current().type == TokenType::RPAREN) {
            if (expectParentheses) {
                stream.advance();
            }
            return parameters;
        }

        do {
            // MYT-110: collect per-parameter `@X` chain before the type.
            auto paramAnnotations = utilities::AnnotationParser::parseAnnotations(stream);

            ValueType paramType = TypeParser::parseType(stream);

            if (stream.current().type != TokenType::IDENTIFIER) {
                throw ParseException("Expected parameter name", stream.location());
            }

            std::string paramName = stream.current().stringValue.getString();
            SourceLocation paramLocation = stream.location();
            NameValidator::validateIdentifierName(paramName, "Parameter", paramLocation);
            stream.advance();

            parameters.push_back({std::move(paramName), paramType, std::move(paramAnnotations)});

            if (stream.current().type == TokenType::COMMA) {
                stream.advance();
            } else {
                break;
            }
        } while (stream.current().type != TokenType::RPAREN);

        if (expectParentheses) {
            stream.expect(TokenType::RPAREN);
        }

        return parameters;
    }

    std::vector<ParameterDeclaration>
    ParameterParser::parseGenericParameterDeclList(TokenStream& stream, bool expectParentheses)
    {
        using namespace value;
        using namespace errors;
        using namespace token;

        std::vector<ParameterDeclaration> parameters;

        if (expectParentheses) {
            stream.expect(TokenType::LPAREN);
        }

        if (stream.current().type == TokenType::RPAREN) {
            if (expectParentheses) {
                stream.advance();
            }
            return parameters;
        }

        do {
            auto paramAnnotations = utilities::AnnotationParser::parseAnnotations(stream);

            auto paramType = TypeParser::parseGenericType(stream);

            if (stream.current().type != TokenType::IDENTIFIER) {
                throw ParseException("Expected parameter name", stream.location());
            }

            std::string paramName = stream.current().stringValue.getString();
            SourceLocation paramLocation = stream.location();
            NameValidator::validateIdentifierName(paramName, "Parameter", paramLocation);
            stream.advance();

            parameters.push_back({std::move(paramName), paramType, std::move(paramAnnotations)});

            if (stream.current().type == TokenType::COMMA) {
                stream.advance();
            } else {
                break;
            }
        } while (stream.current().type != TokenType::RPAREN);

        if (expectParentheses) {
            stream.expect(TokenType::RPAREN);
        }

        return parameters;
    }

    std::vector<TypedParameterDeclaration>
    ParameterParser::parseTypedParameterDeclList(TokenStream& stream, bool expectParentheses)
    {
        using namespace value;
        using namespace errors;
        using namespace token;

        std::vector<TypedParameterDeclaration> parameters;

        if (expectParentheses) {
            stream.expect(TokenType::LPAREN);
        }

        if (stream.current().type == TokenType::RPAREN) {
            if (expectParentheses) {
                stream.advance();
            }
            return parameters;
        }

        do {
            auto paramAnnotations = utilities::AnnotationParser::parseAnnotations(stream);

            TypeInfo typeInfo = TypeParser::parseTypeInfo(stream);
            ParameterType paramType =
                (typeInfo.baseType == ValueType::OBJECT && !typeInfo.className.empty())
                    ? ParameterType::forClass(typeInfo.className)
                    : ParameterType(typeInfo.baseType);
            paramType.nullable = typeInfo.isNullable;

            if (stream.current().type != TokenType::IDENTIFIER) {
                throw ParseException("Expected parameter name", stream.location());
            }

            std::string paramName = stream.current().stringValue.getString();
            SourceLocation paramLocation = stream.location();
            NameValidator::validateIdentifierName(paramName, "Parameter", paramLocation);
            stream.advance();

            parameters.push_back({std::move(paramName), paramType, std::move(paramAnnotations)});

            if (stream.current().type == TokenType::COMMA) {
                stream.advance();
            } else {
                break;
            }
        } while (stream.current().type != TokenType::RPAREN);

        if (expectParentheses) {
            stream.expect(TokenType::RPAREN);
        }

        return parameters;
    }

    // --- Legacy wrappers (discard annotations to preserve old API) ----------

    std::vector<std::pair<std::string, value::ValueType>>
    ParameterParser::parseParameterList(TokenStream& stream, bool expectParentheses)
    {
        auto decls = parseParameterDeclList(stream, expectParentheses);
        std::vector<std::pair<std::string, value::ValueType>> result;
        result.reserve(decls.size());
        for (auto& d : decls) {
            result.emplace_back(std::move(d.name), d.type);
        }
        return result;
    }

    std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>>
    ParameterParser::parseGenericParameterList(TokenStream& stream, bool expectParentheses)
    {
        auto decls = parseGenericParameterDeclList(stream, expectParentheses);
        std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>> result;
        result.reserve(decls.size());
        for (auto& d : decls) {
            result.emplace_back(std::move(d.name), std::move(d.type));
        }
        return result;
    }

    std::vector<std::pair<std::string, value::ParameterType>>
    ParameterParser::parseParameterListWithTypes(TokenStream& stream, bool expectParentheses)
    {
        auto decls = parseTypedParameterDeclList(stream, expectParentheses);
        std::vector<std::pair<std::string, value::ParameterType>> result;
        result.reserve(decls.size());
        for (auto& d : decls) {
            result.emplace_back(std::move(d.name), std::move(d.type));
        }
        return result;
    }
}
