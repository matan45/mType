#include "JsonParser.hpp"
#include <cstdint>
#include <stdexcept>
#include <cstdlib>

namespace json
{
    JsonParser::JsonParser(const std::string& input)
        : input(input), pos(0)
    {
    }

    std::shared_ptr<JsonValue> JsonParser::parse(const std::string& jsonString)
    {
        JsonParser parser(jsonString);
        parser.skipWhitespace();

        if (parser.isAtEnd())
            parser.error("Empty JSON input");

        auto result = parser.parseValue();
        parser.skipWhitespace();

        if (!parser.isAtEnd())
            parser.error("Unexpected characters after JSON value");

        return result;
    }

    std::shared_ptr<JsonValue> JsonParser::parseValue()
    {
        skipWhitespace();
        if (isAtEnd())
            error("Unexpected end of input");

        char c = peek();
        switch (c)
        {
            case '{': return parseObject();
            case '[': return parseArray();
            case '"': return parseString();
            case 't': case 'f': case 'n': return parseLiteral();
            default:
                if (c == '-' || (c >= '0' && c <= '9'))
                    return parseNumber();
                error("Unexpected character: '" + std::string(1, c) + "'");
        }
    }

    std::shared_ptr<JsonValue> JsonParser::parseObject()
    {
        expect('{');
        auto obj = JsonValue::object();
        skipWhitespace();

        if (peek() == '}')
        {
            advance();
            return obj;
        }

        while (true)
        {
            skipWhitespace();
            if (peek() != '"')
                error("Expected string key in object");

            std::string key = parseRawString();
            skipWhitespace();
            expect(':');
            auto value = parseValue();
            obj->setProperty(key, std::move(value));

            skipWhitespace();
            if (peek() == '}')
            {
                advance();
                return obj;
            }
            expect(',');
        }
    }

    std::shared_ptr<JsonValue> JsonParser::parseArray()
    {
        expect('[');
        auto arr = JsonValue::array();
        skipWhitespace();

        if (peek() == ']')
        {
            advance();
            return arr;
        }

        while (true)
        {
            auto value = parseValue();
            arr->addToArray(std::move(value));

            skipWhitespace();
            if (peek() == ']')
            {
                advance();
                return arr;
            }
            expect(',');
        }
    }

    std::shared_ptr<JsonValue> JsonParser::parseString()
    {
        return JsonValue::string(parseRawString());
    }

    std::shared_ptr<JsonValue> JsonParser::parseNumber()
    {
        size_t start = pos;
        bool isFloat = false;

        if (peek() == '-') advance();

        if (peek() == '0')
        {
            advance();
        }
        else if (peek() >= '1' && peek() <= '9')
        {
            while (!isAtEnd() && peek() >= '0' && peek() <= '9')
                advance();
        }
        else
        {
            error("Invalid number");
        }

        if (!isAtEnd() && peek() == '.')
        {
            isFloat = true;
            advance();
            if (isAtEnd() || peek() < '0' || peek() > '9')
                error("Invalid number: expected digit after decimal point");
            while (!isAtEnd() && peek() >= '0' && peek() <= '9')
                advance();
        }

        if (!isAtEnd() && (peek() == 'e' || peek() == 'E'))
        {
            isFloat = true;
            advance();
            if (!isAtEnd() && (peek() == '+' || peek() == '-'))
                advance();
            if (isAtEnd() || peek() < '0' || peek() > '9')
                error("Invalid number: expected digit in exponent");
            while (!isAtEnd() && peek() >= '0' && peek() <= '9')
                advance();
        }

        std::string numStr = input.substr(start, pos - start);

        if (isFloat)
            return JsonValue::floating(std::stod(numStr));

        try
        {
            int64_t val = std::stoll(numStr);
            return JsonValue::integer(val);
        }
        catch (const std::out_of_range&)
        {
            return JsonValue::floating(std::stod(numStr));
        }
    }

    std::shared_ptr<JsonValue> JsonParser::parseLiteral()
    {
        if (input.compare(pos, 4, "true") == 0)
        {
            pos += 4;
            return JsonValue::boolean(true);
        }
        if (input.compare(pos, 5, "false") == 0)
        {
            pos += 5;
            return JsonValue::boolean(false);
        }
        if (input.compare(pos, 4, "null") == 0)
        {
            pos += 4;
            return JsonValue::null();
        }
        error("Invalid literal");
    }

    std::string JsonParser::parseRawString()
    {
        expect('"');
        std::string result;
        result.reserve(32);

        while (!isAtEnd())
        {
            char c = advance();
            if (c == '"')
                return result;
            if (c == '\\')
            {
                result += parseEscapeSequence();
            }
            else
            {
                result += c;
            }
        }
        error("Unterminated string");
    }

    char JsonParser::parseEscapeSequence()
    {
        if (isAtEnd())
            error("Unexpected end of input in escape sequence");

        char c = advance();
        switch (c)
        {
            case '"':  return '"';
            case '\\': return '\\';
            case '/':  return '/';
            case 'b':  return '\b';
            case 'f':  return '\f';
            case 'n':  return '\n';
            case 'r':  return '\r';
            case 't':  return '\t';
            case 'u':
            {
                if (pos + 4 > input.size())
                    error("Invalid unicode escape");
                std::string hex = input.substr(pos, 4);
                pos += 4;
                unsigned long codePoint = std::stoul(hex, nullptr, 16);
                // Simple ASCII range handling
                if (codePoint < 0x80)
                    return static_cast<char>(codePoint);
                // For non-ASCII, return '?' as simplified handling
                return '?';
            }
            default:
                error("Invalid escape sequence: \\" + std::string(1, c));
        }
    }

    void JsonParser::skipWhitespace()
    {
        while (!isAtEnd())
        {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                advance();
            else
                break;
        }
    }

    char JsonParser::peek() const
    {
        if (isAtEnd()) return '\0';
        return input[pos];
    }

    char JsonParser::advance()
    {
        if (isAtEnd())
            error("Unexpected end of input");
        return input[pos++];
    }

    void JsonParser::expect(char c)
    {
        skipWhitespace();
        if (isAtEnd())
            error("Expected '" + std::string(1, c) + "' but reached end of input");
        if (peek() != c)
            error("Expected '" + std::string(1, c) + "' but found '" + std::string(1, peek()) + "'");
        advance();
    }

    bool JsonParser::isAtEnd() const
    {
        return pos >= input.size();
    }

    void JsonParser::error(const std::string& message) const
    {
        throw std::runtime_error("JSON parse error at position " +
                                 std::to_string(pos) + ": " + message);
    }
}
