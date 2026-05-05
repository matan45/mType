#pragma once
#include "JsonValue.hpp"
#include <cstddef>
#include <string>

namespace json
{
    class JsonParser
    {
    public:
        static std::shared_ptr<JsonValue> parse(const std::string& jsonString);

    private:
        std::string input;
        size_t pos;

        explicit JsonParser(const std::string& input);

        std::shared_ptr<JsonValue> parseValue();
        std::shared_ptr<JsonValue> parseObject();
        std::shared_ptr<JsonValue> parseArray();
        std::shared_ptr<JsonValue> parseString();
        std::shared_ptr<JsonValue> parseNumber();
        std::shared_ptr<JsonValue> parseLiteral();

        void skipWhitespace();
        char peek() const;
        char advance();
        void expect(char c);
        std::string parseRawString();
        char parseEscapeSequence();
        bool isAtEnd() const;

        [[noreturn]] void error(const std::string& message) const;
    };
}
