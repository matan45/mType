#include "DebugExpressionEvaluator.hpp"
#include "DebugExpressionEvaluator_Internal.hpp"
#include "VMVariableInspector.hpp"
#include <cctype>
#include <cmath>
#include <stdexcept>
#include "../value/ValueShim.hpp"

namespace debugger
{
    namespace
    {
        using detail::Token;
        using detail::TokenType;

        class Lexer
        {
        public:
            explicit Lexer(std::string source) : source_(std::move(source)) {}

            std::vector<Token> tokenize()
            {
                std::vector<Token> tokens;

                while (!isAtEnd())
                {
                    char c = peek();
                    if (std::isspace(static_cast<unsigned char>(c)))
                    {
                        advance();
                        continue;
                    }

                    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
                    {
                        tokens.push_back(readIdentifier());
                        continue;
                    }

                    if (std::isdigit(static_cast<unsigned char>(c)))
                    {
                        tokens.push_back(readNumber());
                        continue;
                    }

                    if (c == '"')
                    {
                        tokens.push_back(readString());
                        continue;
                    }

                    tokens.push_back(readOperator());
                }

                tokens.push_back({TokenType::End, ""});
                return tokens;
            }

        private:
            bool isAtEnd() const { return pos_ >= source_.size(); }
            char peek() const { return isAtEnd() ? '\0' : source_[pos_]; }
            char peekNext() const { return pos_ + 1 < source_.size() ? source_[pos_ + 1] : '\0'; }
            char advance() { return source_[pos_++]; }

            Token readIdentifier()
            {
                size_t start = pos_;
                while (!isAtEnd() &&
                       (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_'))
                {
                    advance();
                }
                return {TokenType::Identifier, source_.substr(start, pos_ - start)};
            }

            Token readNumber()
            {
                size_t start = pos_;
                while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek())))
                {
                    advance();
                }

                bool isFloat = false;
                if (!isAtEnd() && peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext())))
                {
                    isFloat = true;
                    advance();
                    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek())))
                    {
                        advance();
                    }
                }

                return {isFloat ? TokenType::Float : TokenType::Integer,
                        source_.substr(start, pos_ - start)};
            }

            Token readString()
            {
                advance();
                std::string value;
                bool escaped = false;

                while (!isAtEnd())
                {
                    char c = advance();
                    if (escaped)
                    {
                        switch (c)
                        {
                        case 'n': value.push_back('\n'); break;
                        case 'r': value.push_back('\r'); break;
                        case 't': value.push_back('\t'); break;
                        case '"': value.push_back('"'); break;
                        case '\\': value.push_back('\\'); break;
                        default: value.push_back(c); break;
                        }
                        escaped = false;
                    }
                    else if (c == '\\')
                    {
                        escaped = true;
                    }
                    else if (c == '"')
                    {
                        return {TokenType::String, value};
                    }
                    else
                    {
                        value.push_back(c);
                    }
                }

                throw std::runtime_error("Unterminated string literal");
            }

            Token readOperator()
            {
                char c = advance();
                char next = peek();

                switch (c)
                {
                case '(':  return {TokenType::LParen, "("};
                case ')':  return {TokenType::RParen, ")"};
                case '[':  return {TokenType::LBracket, "["};
                case ']':  return {TokenType::RBracket, "]"};
                case '.':  return {TokenType::Dot, "."};
                case ':':
                    if (next == ':')
                    {
                        advance();
                        return {TokenType::Scope, "::"};
                    }
                    break;
                case '+':
                    if (next == '=') { advance(); return {TokenType::Assign, "+="}; }
                    return {TokenType::Plus, "+"};
                case '-':
                    if (next == '=') { advance(); return {TokenType::Assign, "-="}; }
                    return {TokenType::Minus, "-"};
                case '*':
                    if (next == '=') { advance(); return {TokenType::Assign, "*="}; }
                    return {TokenType::Star, "*"};
                case '/':
                    if (next == '=') { advance(); return {TokenType::Assign, "/="}; }
                    return {TokenType::Slash, "/"};
                case '%':
                    if (next == '=') { advance(); return {TokenType::Assign, "%="}; }
                    return {TokenType::Percent, "%"};
                case '!':
                    if (next == '=') { advance(); return {TokenType::BangEqual, "!="}; }
                    return {TokenType::Bang, "!"};
                case '<':
                    if (next == '=') { advance(); return {TokenType::LessEqual, "<="}; }
                    return {TokenType::Less, "<"};
                case '>':
                    if (next == '=') { advance(); return {TokenType::GreaterEqual, ">="}; }
                    return {TokenType::Greater, ">"};
                case '=':
                    if (next == '=') { advance(); return {TokenType::EqualEqual, "=="}; }
                    return {TokenType::Assign, "="};
                case '&':
                    if (next == '&') { advance(); return {TokenType::AndAnd, "&&"}; }
                    break;
                case '|':
                    if (next == '|') { advance(); return {TokenType::OrOr, "||"}; }
                    break;
                default:
                    break;
                }

                throw std::runtime_error(std::string("Unexpected token '") + c + "'");
            }

            std::string source_;
            size_t pos_ = 0;
        };
    }

    DebugExpressionEvaluator::Result DebugExpressionEvaluator::evaluate(
        std::shared_ptr<vm::runtime::VirtualMachine> vm,
        VMVariableInspector& inspector,
        const std::string& expression)
    {
        return evaluateWithResolver(expression, [&](const std::string& name) {
            return inspector.findVariableValue(vm, name);
        });
    }

    DebugExpressionEvaluator::Result DebugExpressionEvaluator::evaluateWithResolver(
        const std::string& expression,
        const Resolver& resolver)
    {
        try
        {
            Lexer lexer(expression);
            value::Value result = detail::parseTokens(lexer.tokenize(), resolver);
            return {true, result, ""};
        }
        catch (const std::exception& e)
        {
            return {false, std::monostate{}, e.what()};
        }
    }

    bool DebugExpressionEvaluator::isTruthy(const value::Value& val)
    {
        if (value::isBool(val)) return value::asBool(val);
        if (value::isInt(val)) return value::asInt(val) != 0;
        if (value::isFloat(val)) return value::asFloat(val) != 0.0 && !std::isnan(value::asFloat(val));
        // MYT-317: SSO-aware truthiness.
        if (value::isAnyString(val)) return !value::asStringView(val).empty();
        return !(value::isVoid(val) || value::isNullType(val));
    }
}
