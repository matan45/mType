#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <array>
#include <string_view>
#include "../token/TokenType.hpp"
#include "../token/Token.hpp"
#include "../services/FileReader.hpp"
#include "SourceLocationTracker.hpp"
#include "BracketBalancer.hpp"

namespace lexer
{
    using namespace token;

    class Lexer
    {
    private:
        std::string input;
        size_t pos;
        std::unique_ptr<FileReader> fileReader;

        // Separated concerns
        std::unique_ptr<SourceLocationTracker> locationTracker;
        std::unique_ptr<BracketBalancer> bracketBalancer;

        // Operator information structure
        struct OperatorInfo
        {
            std::string_view symbol;
            TokenType type;
            size_t length;

            constexpr OperatorInfo(std::string_view sym, TokenType t, size_t len)
                : symbol(sym), type(t), length(len)
            {
            }
        };

        // Operator lookup table declarations
        static const std::array<OperatorInfo, 15> TWO_CHAR_OPERATORS;
        static const std::array<OperatorInfo, 20> SINGLE_CHAR_OPERATORS;

        // List of keywords and their corresponding TokenType
        std::unordered_map<std::string, TokenType> keywords = {
            {"function", TokenType::FUNCTION},
            {"return", TokenType::RETURN},
            {"if", TokenType::IF},
            {"else", TokenType::ELSE},
            {"while", TokenType::WHILE},
            {"do", TokenType::DO},
            {"for", TokenType::FOR},
            {"final", TokenType::FINAL},
            {"break", TokenType::BREAK},
            {"continue", TokenType::CONTINUE},
            {"int", TokenType::INT},
            {"float", TokenType::FLOAT},
            {"bool", TokenType::BOOL},
            {"string", TokenType::STRING_TYPE},
            {"void", TokenType::VOID},
            {"class", TokenType::CLASS},
            {"new", TokenType::NEW},
            {"static", TokenType::STATIC},
            {"constructor", TokenType::CONSTRUCTOR},
            {"null", TokenType::NULL_LITERAL},
            {"true", TokenType::TRUE},
            {"native", TokenType::NATIVE},
            {"import", TokenType::IMPORT},
            {"false", TokenType::FALSE},
            {"switch", TokenType::SWITCH},
            {"case", TokenType::CASE},
            {"interface", TokenType::INTERFACE},
            {"implements", TokenType::IMPLEMENTS},
            {"extends", TokenType::EXTENDS},
            {"default", TokenType::DEFAULT}
        };

    public:
        explicit Lexer(const std::string& filePath = "<unknown>",
                       std::unique_ptr<FileReader> reader = std::make_unique<FileReader>());

        // Non-copyable but movable
        Lexer(const Lexer&) = delete;
        Lexer& operator=(const Lexer&) = delete;
        Lexer(Lexer&&) = default;
        Lexer& operator=(Lexer&&) = default;

        Token getNextToken();
        Token peekNextToken();

    private:
        // Core parsing methods
        float parseFloat();
        int parseInteger();
        std::string_view parseIdentifier();
        std::string parseStringLiteral();
        void skipWhitespaceAndComments();

        // Movement and positioning
        void advance();
        void advanceMultiple(size_t count);

        // Token creation helpers
        Token tryParseOperator();
        Token tryParseSpacedOperator();
        TokenType findKeywordType(std::string_view identifier) const;

        // Error handling
        [[noreturn]] void throwError(const std::string& message);
    };
}
