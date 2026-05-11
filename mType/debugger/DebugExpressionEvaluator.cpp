#include "DebugExpressionEvaluator.hpp"
#include "VMVariableInspector.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../value/FlatMultiArray.hpp"
#include "../value/NativeArray.hpp"
#include "../value/SparseMultiArray.hpp"
#include "../value/StringPool.hpp"
#include "../value/ValueShim.hpp"
#include "../value/arrays/object/FlatMultiObjectArray.hpp"
#include "../vm/runtime/utils/CheckedArithmetic.hpp"

#include <cctype>
#include <cstdint>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace debugger
{
    namespace
    {
        enum class TokenType
        {
            End,
            Identifier,
            Integer,
            Float,
            String,
            LParen,
            RParen,
            LBracket,
            RBracket,
            Dot,
            Scope,
            Plus,
            Minus,
            Star,
            Slash,
            Percent,
            Bang,
            Less,
            LessEqual,
            Greater,
            GreaterEqual,
            EqualEqual,
            BangEqual,
            AndAnd,
            OrOr,
            Assign
        };

        struct Token
        {
            TokenType type;
            std::string text;
        };

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
                case '(':
                    return {TokenType::LParen, "("};
                case ')':
                    return {TokenType::RParen, ")"};
                case '[':
                    return {TokenType::LBracket, "["};
                case ']':
                    return {TokenType::RBracket, "]"};
                case '.':
                    return {TokenType::Dot, "."};
                case ':':
                    if (next == ':')
                    {
                        advance();
                        return {TokenType::Scope, "::"};
                    }
                    break;
                case '+':
                    if (next == '=')
                    {
                        advance();
                        return {TokenType::Assign, "+="};
                    }
                    return {TokenType::Plus, "+"};
                case '-':
                    if (next == '=')
                    {
                        advance();
                        return {TokenType::Assign, "-="};
                    }
                    return {TokenType::Minus, "-"};
                case '*':
                    if (next == '=')
                    {
                        advance();
                        return {TokenType::Assign, "*="};
                    }
                    return {TokenType::Star, "*"};
                case '/':
                    if (next == '=')
                    {
                        advance();
                        return {TokenType::Assign, "/="};
                    }
                    return {TokenType::Slash, "/"};
                case '%':
                    if (next == '=')
                    {
                        advance();
                        return {TokenType::Assign, "%="};
                    }
                    return {TokenType::Percent, "%"};
                case '!':
                    if (next == '=')
                    {
                        advance();
                        return {TokenType::BangEqual, "!="};
                    }
                    return {TokenType::Bang, "!"};
                case '<':
                    if (next == '=')
                    {
                        advance();
                        return {TokenType::LessEqual, "<="};
                    }
                    return {TokenType::Less, "<"};
                case '>':
                    if (next == '=')
                    {
                        advance();
                        return {TokenType::GreaterEqual, ">="};
                    }
                    return {TokenType::Greater, ">"};
                case '=':
                    if (next == '=')
                    {
                        advance();
                        return {TokenType::EqualEqual, "=="};
                    }
                    return {TokenType::Assign, "="};
                case '&':
                    if (next == '&')
                    {
                        advance();
                        return {TokenType::AndAnd, "&&"};
                    }
                    break;
                case '|':
                    if (next == '|')
                    {
                        advance();
                        return {TokenType::OrOr, "||"};
                    }
                    break;
                default:
                    break;
                }

                throw std::runtime_error(std::string("Unexpected token '") + c + "'");
            }

            std::string source_;
            size_t pos_ = 0;
        };

        bool isNullish(const value::Value& val)
        {
            return value::isVoid(val) || value::isNullType(val);
        }

        bool isNumeric(const value::Value& val)
        {
            return value::isInt(val) || value::isFloat(val);
        }

        double asDouble(const value::Value& val)
        {
            if (value::isFloat(val)) return value::asFloat(val);
            if (value::isInt(val)) return static_cast<double>(value::asInt(val));
            throw std::runtime_error("Expected numeric value");
        }

        std::string asPlainString(const value::Value& val)
        {
            if (value::isString(val)) return value::asString(val);
            if (value::isInternedString(val)) return value::asInternedString(val).getString();
            throw std::runtime_error("Expected string value");
        }

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
                    bool equal = equals(left, right);
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
                    left = compare(left, right, op);
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
                    left = (op == TokenType::Plus) ? add(left, right) : subtract(left, right);
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
                    left = multiplyDivideModulo(left, right, op);
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
                        value = readField(value, field.text);
                        continue;
                    }

                    if (match(TokenType::LBracket))
                    {
                        value::Value index = parseOr();
                        consume(TokenType::RBracket, "Expected ']' after array index");
                        value = readIndex(value, index);
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

            static bool equals(const value::Value& left, const value::Value& right)
            {
                if (isNullish(left) || isNullish(right))
                {
                    return isNullish(left) && isNullish(right);
                }
                if (isNumeric(left) && isNumeric(right))
                {
                    return asDouble(left) == asDouble(right);
                }
                if ((value::isString(left) || value::isInternedString(left)) &&
                    (value::isString(right) || value::isInternedString(right)))
                {
                    return asPlainString(left) == asPlainString(right);
                }
                return left == right;
            }

            static value::Value compare(const value::Value& left,
                                        const value::Value& right,
                                        TokenType op)
            {
                if (!isNumeric(left) || !isNumeric(right))
                {
                    throw std::runtime_error("Comparison requires numeric operands");
                }

                double l = asDouble(left);
                double r = asDouble(right);
                switch (op)
                {
                case TokenType::Less: return l < r;
                case TokenType::LessEqual: return l <= r;
                case TokenType::Greater: return l > r;
                case TokenType::GreaterEqual: return l >= r;
                default: break;
                }
                throw std::runtime_error("Invalid comparison operator");
            }

            static value::Value add(const value::Value& left, const value::Value& right)
            {
                if (value::isInt(left) && value::isInt(right))
                {
                    return vm::runtime::utils::wrappingAdd64(value::asInt(left), value::asInt(right));
                }
                if (isNumeric(left) && isNumeric(right))
                {
                    return asDouble(left) + asDouble(right);
                }
                if ((value::isString(left) || value::isInternedString(left)) &&
                    (value::isString(right) || value::isInternedString(right)))
                {
                    auto& pool = value::StringPool::getInstance();
                    return pool.intern(asPlainString(left) + asPlainString(right));
                }
                throw std::runtime_error("'+' requires numeric operands or two strings");
            }

            static value::Value subtract(const value::Value& left, const value::Value& right)
            {
                if (value::isInt(left) && value::isInt(right))
                {
                    return vm::runtime::utils::wrappingSub64(value::asInt(left), value::asInt(right));
                }
                if (isNumeric(left) && isNumeric(right))
                {
                    return asDouble(left) - asDouble(right);
                }
                throw std::runtime_error("'-' requires numeric operands");
            }

            static value::Value multiplyDivideModulo(const value::Value& left,
                                                     const value::Value& right,
                                                     TokenType op)
            {
                if (!isNumeric(left) || !isNumeric(right))
                {
                    throw std::runtime_error("Arithmetic requires numeric operands");
                }

                if (op == TokenType::Percent)
                {
                    if (!value::isInt(left) || !value::isInt(right))
                    {
                        throw std::runtime_error("'%' requires integer operands");
                    }
                    int64_t r = value::asInt(right);
                    if (r == 0) throw std::runtime_error("Modulo by zero");
                    int64_t l = value::asInt(left);
                    if (l == INT64_MIN && r == -1) return static_cast<int64_t>(0);
                    return l % r;
                }

                if (value::isInt(left) && value::isInt(right))
                {
                    int64_t l = value::asInt(left);
                    int64_t r = value::asInt(right);
                    if (op == TokenType::Star)
                    {
                        return vm::runtime::utils::wrappingMul64(l, r);
                    }
                    if (r == 0) throw std::runtime_error("Division by zero");
                    if (l == INT64_MIN && r == -1) return INT64_MIN;
                    return l / r;
                }

                double l = asDouble(left);
                double r = asDouble(right);
                if (op == TokenType::Star)
                {
                    return l * r;
                }
                if (r == 0.0)
                {
                    throw std::runtime_error("Division by zero");
                }
                return l / r;
            }

            static value::Value readField(const value::Value& receiver, const std::string& fieldName)
            {
                if (!value::isAnyObject(receiver))
                {
                    throw std::runtime_error("Field access requires an object receiver");
                }

                auto* obj = value::asObjectInstanceRaw(receiver);
                if (!obj)
                {
                    throw std::runtime_error("Field access on null object");
                }

                value::Value field = obj->getFieldValue(fieldName);
                if (value::isVoid(field))
                {
                    throw std::runtime_error("Field not found: " + fieldName);
                }
                return field;
            }

            static value::Value readIndex(const value::Value& receiver, const value::Value& indexValue)
            {
                if (!value::isInt(indexValue))
                {
                    throw std::runtime_error("Array index must be an integer");
                }

                int64_t signedIndex = value::asInt(indexValue);
                if (signedIndex < 0)
                {
                    throw std::runtime_error("Array index cannot be negative");
                }
                size_t index = static_cast<size_t>(signedIndex);

                if (value::isNativeArray(receiver))
                {
                    auto arr = value::asNativeArray(receiver);
                    if (!arr || index >= arr->size())
                    {
                        throw std::runtime_error("Array index out of bounds");
                    }
                    return arr->get(index);
                }
                if (value::isFlatMultiArray(receiver))
                {
                    auto arr = value::asFlatMultiArray(receiver);
                    return arr->get(index);
                }
                if (value::isSparseMultiArray(receiver))
                {
                    auto arr = value::asSparseMultiArray(receiver);
                    return arr->get(index);
                }
                if (value::isFlatMultiObjectArray(receiver))
                {
                    auto arr = value::asFlatMultiObjectArray(receiver);
                    return arr->getLinear(index);
                }

                throw std::runtime_error("Index access requires an array receiver");
            }

            std::vector<Token> tokens_;
            DebugExpressionEvaluator::Resolver resolver_;
            size_t current_ = 0;
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
            Parser parser(lexer.tokenize(), resolver);
            return {true, parser.parse(), ""};
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
        if (value::isString(val)) return !value::asString(val).empty();
        if (value::isInternedString(val)) return !value::asInternedString(val).getString().empty();
        return !isNullish(val);
    }
}
