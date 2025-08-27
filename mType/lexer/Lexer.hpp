#pragma once
#include <string>
#include <unordered_map>
#include <stack>
#include "../token/TokenType.hpp"
#include "../token/Token.hpp"

namespace lexer
{
    using namespace token;
    class Lexer
    {
    private:
        std::string input;
        std::string filename;
        size_t pos;
        int currentLine;
        int currentColumn;
        std::stack<char> balanceStack; // Stack for keeping track of parentheses and braces
        std::vector<std::string> lines;

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
            {"namespace", TokenType::NAMESPACE},
            {"using", TokenType::USING},
            {"false", TokenType::FALSE},
            {"switch", TokenType::SWITCH},
            {"case", TokenType::CASE},
            {"default", TokenType::DEFAULT}
        };

    public:
        explicit Lexer(const std::string& input, const std::string& fname = "<unknown>");

        Token getNextToken();
        Token peekNextToken();
    private:
        void splitIntoLines();
        
        float parseFloat();
        int parseInteger();
        std::string parseIdentifier();
        std::string parseStringLiteral();

        void skipWhitespaceAndComments();

        void advance();
        void throwError(const std::string& message);
    };
}

