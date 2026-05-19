#include "DebugExpressionEvaluator_Internal.hpp"
#include <cstdint>
#include <stdexcept>

namespace debugger
{
    namespace
    {
        using detail::Token;
        using detail::TokenType;

        class Parser
        {
        public:
            Parser(std::vector<Token> tokens, DebugExpressionEvaluator::Resolver resolver)
                : tokens_(std::move(tokens)), resolver_(std::move(resolver))
            {
            }

            value::Value parse()
            {
                if (check(TokenType::End))
                {
                    throw std::runtime_error("Empty expression");
                }

                value::Value result = parseOr();
                if (!check(TokenType::End))
                {
                    if (match(TokenType::Assign))
                    {
                        throw std::runtime_error("Assignments are not supported in debugger expressions");
                    }
                    if (match(TokenType::LParen))
                    {
                        throw std::runtime_error("Method/function calls are not supported in debugger expressions");
                    }
                    throw std::runtime_error("Unexpected token '" + peek().text + "'");
                }
                return result;
            }

        private:
            bool check(TokenType type) const { return peek().type == type; }

            bool match(TokenType type)
            {
                if (!check(type)) return false;
                advance();
                return true;
            }

            const Token& peek() const { return tokens_[current_]; }
            const Token& previous() const { return tokens_[current_ - 1]; }
            const Token& advance()
            {
                if (!check(TokenType::End)) ++current_;
                return previous();
            }

            void consume(TokenType type, const std::string& message)
            {
                if (match(type)) return;
                throw std::runtime_error(message);
            }

            value::Value parseOr()
            {
                value::Value left = parseAnd();
                while (match(TokenType::OrOr))
                {
                    if (DebugExpressionEvaluator::isTruthy(left))
                    {
                        parseAnd();
                        left = true;
                    }
                    else
                    {
                        left = DebugExpressionEvaluator::isTruthy(parseAnd());
                    }
                }
                return left;
            }

            value::Value parseAnd()
            {
                value::Value left = parseEquality();
                while (match(TokenType::AndAnd))
                {
                    if (!DebugExpressionEvaluator::isTruthy(left))
                    {
                        parseEquality();
                        left = false;
                    }
                    else
                    {
                        left = DebugExpressionEvaluator::isTruthy(parseEquality());
                    }
                }
                return left;
            }

            value::Value parseEquality()
            {
                value::Value left = parseComparison();
                while (match(TokenType::EqualEqual) || match(TokenType::BangEqual))
                {
                    TokenType op = previous().type;
                    value::Value right = parseComparison();
                    bool equal = detail::equals(left, right);
                    left = (op == TokenType::EqualEqual) ? equal : !equal;
                }
                return left;
            }

            value::Value parseComparison()
            {
                value::Value left = parseTerm();
                while (match(TokenType::Less) || match(TokenType::LessEqual) ||
                       match(TokenType::Greater) || match(TokenType::GreaterEqual))
                {
                    TokenType op = previous().type;
                    value::Value right = parseTerm();
                    left = detail::compare(left, right, op);
                }
                return left;
            }

            value::Value parseTerm()
            {
                value::Value left = parseFactor();
                while (match(TokenType::Plus) || match(TokenType::Minus))
                {
                    TokenType op = previous().type;
                    value::Value right = parseFactor();
                    left = (op == TokenType::Plus) ? detail::add(left, right) : detail::subtract(left, right);
                }
                return left;
            }

            value::Value parseFactor()
            {
                value::Value left = parseUnary();
                while (match(TokenType::Star) || match(TokenType::Slash) || match(TokenType::Percent))
                {
                    TokenType op = previous().type;
                    value::Value right = parseUnary();
                    left = detail::multiplyDivideModulo(left, right, op);
                }
                return left;
            }

            value::Value parseUnary()
            {
                if (match(TokenType::Bang))
                {
                    return !DebugExpressionEvaluator::isTruthy(parseUnary());
                }

                if (match(TokenType::Minus))
                {
                    value::Value val = parseUnary();
                    if (value::isInt(val)) return vm::runtime::utils::wrappingNeg64(value::asInt(val));
                    if (value::isFloat(val)) return -value::asFloat(val);
                    throw std::runtime_error("Unary '-' requires a numeric value");
                }

                return parsePostfix();
            }

            value::Value parsePostfix()
            {
                value::Value value = parsePrimary();

                while (true)
                {
                    if (match(TokenType::LParen))
                    {
                        throw std::runtime_error("Method/function calls are not supported in debugger expressions");
                    }

                    if (match(TokenType::Dot))
                    {
                        Token field = advance();
                        if (field.type != TokenType::Identifier)
                        {
                            throw std::runtime_error("Expected field name after '.'");
                        }
                        if (check(TokenType::LParen))
                        {
                            throw std::runtime_error("Method/function calls are not supported in debugger expressions");
                        }
                        value = detail::readField(value, field.text);
                        continue;
                    }

                    if (match(TokenType::LBracket))
                    {
                        value::Value index = parseOr();
                        consume(TokenType::RBracket, "Expected ']' after array index");
                        value = detail::readIndex(value, index);
                        continue;
                    }

                    break;
                }

                return value;
            }

            value::Value parsePrimary()
            {
                if (match(TokenType::Integer))
                {
                    return static_cast<int64_t>(std::stoll(previous().text));
                }
                if (match(TokenType::Float))
                {
                    return std::stod(previous().text);
                }
                if (match(TokenType::String))
                {
                    return previous().text;
                }
                if (match(TokenType::Identifier))
                {
                    std::string name = previous().text;
                    if (name == "true") return true;
                    if (name == "false") return false;
                    if (name == "null") return nullptr;
                    if (name == "new" || name == "await")
                    {
                        throw std::runtime_error("'" + name + "' is not supported in debugger expressions");
                    }

                    if (match(TokenType::Scope))
                    {
                        Token field = advance();
                        if (field.type != TokenType::Identifier)
                        {
                            throw std::runtime_error("Expected static field name after '::'");
                        }
                        name += "::" + field.text;
                    }

                    if (check(TokenType::LParen))
                    {
                        throw std::runtime_error("Method/function calls are not supported in debugger expressions");
                    }

                    auto resolved = resolver_(name);
                    if (!resolved)
                    {
                        throw std::runtime_error("Variable not found: " + name);
                    }
                    return *resolved;
                }

                if (match(TokenType::LParen))
                {
                    value::Value value = parseOr();
                    consume(TokenType::RParen, "Expected ')' after expression");
                    return value;
                }

                if (match(TokenType::Assign))
                {
                    throw std::runtime_error("Assignments are not supported in debugger expressions");
                }

                throw std::runtime_error("Expected expression");
            }

            std::vector<Token> tokens_;
            DebugExpressionEvaluator::Resolver resolver_;
            size_t current_ = 0;
        };
    }

    namespace detail
    {
        value::Value parseTokens(std::vector<Token> tokens,
                                 const DebugExpressionEvaluator::Resolver& resolver)
        {
            Parser parser(std::move(tokens), resolver);
            return parser.parse();
        }
    }
}
