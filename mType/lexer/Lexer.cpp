#include "Lexer.hpp"
#include "../errors/ParseException.hpp"
#include <cctype>
#include <stdexcept>

namespace lexer
{
    Lexer::Lexer(const std::string& input, const std::string& fname)
        : input(input), filename(fname), pos(0), currentLine(1), currentColumn(1)
    {
        // Skip UTF-8 BOM if present
        if (input.length() >= 3 &&
            static_cast<unsigned char>(input[0]) == 0xEF &&
            static_cast<unsigned char>(input[1]) == 0xBB &&
            static_cast<unsigned char>(input[2]) == 0xBF)
        {
            pos = 3;
        }

        splitIntoLines();
    }

    Token Lexer::getNextToken()
    {
        skipWhitespaceAndComments();

        if (pos >= input.length())
        {
            return Token{TokenType::END, 0.0f, 0, "", errors::SourceLocation(filename, currentLine, currentColumn)};
        }

        char current = input[pos];
        errors::SourceLocation location(filename, currentLine, currentColumn);

        // Numbers
        if (std::isdigit(current))
        {
            // Look ahead to determine if this is a float or integer
            // Check if there's a decimal point followed by digits in the number
            size_t lookAhead = pos;
            while (lookAhead < input.length() && std::isdigit(input[lookAhead]))
            {
                lookAhead++;
            }
            
            // Check if we found a decimal point followed by digits
            if (lookAhead < input.length() && input[lookAhead] == '.' && 
                lookAhead + 1 < input.length() && std::isdigit(input[lookAhead + 1]))
            {
                float value = parseFloat();
                return Token{TokenType::FLOAT_NUMBER, value, 0, "", location};
            }
            else
            {
                int value = parseInteger();
                return Token{TokenType::INT_NUMBER, 0.0f, value, "", location};
            }
        }

        // Identifiers and keywords
        if (std::isalpha(current) || current == '_')
        {
            std::string identifier = parseIdentifier();
            auto keywordIt = keywords.find(identifier);
            if (keywordIt != keywords.end())
            {
                return Token{keywordIt->second, 0.0f, 0, identifier, location};
            }
            return Token{TokenType::IDENTIFIER, 0.0f, 0, identifier, location};
        }

        // String literals
        if (current == '"')
        {
            std::string value = parseStringLiteral();
            return Token{TokenType::STRING_LITERAL, 0.0f, 0, value, location};
        }

        // Two-character operators
        if (pos + 1 < input.length())
        {
            std::string twoChar = input.substr(pos, 2);
            if (twoChar == "++")
            {
                advance();
                advance();
                return Token{TokenType::INCREMENT, 0.0f, 0, "++", location};
            }
            if (twoChar == "--")
            {
                advance();
                advance();
                return Token{TokenType::DECREMENT, 0.0f, 0, "--", location};
            }
            if (twoChar == "==")
            {
                advance();
                advance();
                return Token{TokenType::EQUALS, 0.0f, 0, "==", location};
            }
            if (twoChar == "!=")
            {
                advance();
                advance();
                return Token{TokenType::NOT_EQUALS, 0.0f, 0, "!=", location};
            }
            if (twoChar == "<=")
            {
                advance();
                advance();
                return Token{TokenType::LESS_EQUALS, 0.0f, 0, "<=", location};
            }
            if (twoChar == ">=")
            {
                advance();
                advance();
                return Token{TokenType::GREATER_EQUALS, 0.0f, 0, ">=", location};
            }
            if (twoChar == "&&")
            {
                advance();
                advance();
                return Token{TokenType::AND, 0.0f, 0, "&&", location};
            }
            if (twoChar == "||")
            {
                advance();
                advance();
                return Token{TokenType::OR, 0.0f, 0, "||", location};
            }
            if (twoChar == "+=")
            {
                advance();
                advance();
                return Token{TokenType::PLUS_ASSIGN, 0.0f, 0, "+=", location};
            }
            if (twoChar == "-=")
            {
                advance();
                advance();
                return Token{TokenType::MINUS_ASSIGN, 0.0f, 0, "-=", location};
            }
            if (twoChar == "*=")
            {
                advance();
                advance();
                return Token{TokenType::MULTIPLY_ASSIGN, 0.0f, 0, "*=", location};
            }
            if (twoChar == "/=")
            {
                advance();
                advance();
                return Token{TokenType::DIVIDE_ASSIGN, 0.0f, 0, "/=", location};
            }
            if (twoChar == "%=")
            {
                advance();
                advance();
                return Token{TokenType::MODULO_ASSIGN, 0.0f, 0, "%=", location};
            }
            if (twoChar == "::")
            {
                advance();
                advance();
                return Token{TokenType::SCOPE, 0.0f, 0, "::", location};
            }
        }

        // Single character operators and punctuation
        switch (current)
        {
        case '+': advance();
            return Token{TokenType::PLUS, 0.0f, 0, "+", location};
        case '-': advance();
            return Token{TokenType::MINUS, 0.0f, 0, "-", location};
        case '*': advance();
            return Token{TokenType::MULTIPLY, 0.0f, 0, "*", location};
        case '/': advance();
            return Token{TokenType::DIVIDE, 0.0f, 0, "/", location};
        case '%': advance();
            return Token{TokenType::MODULO, 0.0f, 0, "%", location};
        case '=': advance();
            return Token{TokenType::ASSIGN, 0.0f, 0, "=", location};
        case '(': advance();
            balanceStack.push('(');
            return Token{TokenType::LPAREN, 0.0f, 0, "(", location};
        case ')':
            if (!balanceStack.empty() && balanceStack.top() == '(') balanceStack.pop();
            else throwError("Unmatched closing parenthesis");
            advance();
            return Token{TokenType::RPAREN, 0.0f, 0, ")", location};
        case '{': advance();
            balanceStack.push('{');
            return Token{TokenType::LBRACE, 0.0f, 0, "{", location};
        case '}':
            if (!balanceStack.empty() && balanceStack.top() == '{') balanceStack.pop();
            else throwError("Unmatched closing brace");
            advance();
            return Token{TokenType::RBRACE, 0.0f, 0, "}", location};
        case ',': advance();
            return Token{TokenType::COMMA, 0.0f, 0, ",", location};
        case ';': advance();
            return Token{TokenType::SEMICOLON, 0.0f, 0, ";", location};
        case ':': advance();
            return Token{TokenType::COLON, 0.0f, 0, ":", location};
        case '?': advance();
            return Token{TokenType::QUESTION, 0.0f, 0, "?", location};
        case '!': advance();
            return Token{TokenType::NOT, 0.0f, 0, "!", location};
        case '<': advance();
            return Token{TokenType::LESS, 0.0f, 0, "<", location};
        case '>': advance();
            return Token{TokenType::GREATER, 0.0f, 0, ">", location};
        case '.': advance();
            return Token{TokenType::DOT, 0.0f, 0, ".", location};
        default:
            throwError("Unexpected character: " + std::string(1, current));
        }
    }

    Token Lexer::peekNextToken()
    {
        size_t savedPos = pos;
        int savedLine = currentLine;
        int savedColumn = currentColumn;
        std::stack<char> savedStack = balanceStack;

        Token token = getNextToken();

        pos = savedPos;
        currentLine = savedLine;
        currentColumn = savedColumn;
        balanceStack = savedStack;

        return token;
    }

    void Lexer::splitIntoLines()
    {
        std::string line;
        for (size_t i = 0; i < input.length(); ++i)
        {
            if (input[i] == '\n')
            {
                lines.push_back(line);
                line.clear();
            }
            else if (input[i] != '\r')
            {
                line += input[i];
            }
        }
        if (!line.empty())
        {
            lines.push_back(line);
        }
    }

    float Lexer::parseFloat()
    {
        size_t start = pos;
        while (pos < input.length() && (std::isdigit(input[pos]) || input[pos] == '.'))
        {
            advance();
        }
        std::string floatStr = input.substr(start, pos - start);
        return std::stof(floatStr);
    }

    int Lexer::parseInteger()
    {
        size_t start = pos;
        while (pos < input.length() && std::isdigit(input[pos]))
        {
            advance();
        }
        std::string intStr = input.substr(start, pos - start);
        return std::stoi(intStr);
    }

    std::string Lexer::parseIdentifier()
    {
        size_t start = pos;
        while (pos < input.length() && (std::isalnum(input[pos]) || input[pos] == '_'))
        {
            advance();
        }
        return input.substr(start, pos - start);
    }

    std::string Lexer::parseStringLiteral()
    {
        advance(); // Skip opening quote
        std::string result;

        while (pos < input.length() && input[pos] != '"')
        {
            if (input[pos] == '\\' && pos + 1 < input.length())
            {
                advance(); // Skip backslash
                switch (input[pos])
                {
                case 'n': result += '\n';
                    break;
                case 't': result += '\t';
                    break;
                case 'r': result += '\r';
                    break;
                case '\\': result += '\\';
                    break;
                case '"': result += '"';
                    break;
                default:
                    result += '\\';
                    result += input[pos];
                    break;
                }
            }
            else
            {
                result += input[pos];
            }
            advance();
        }

        if (pos >= input.length())
        {
            throwError("Unterminated string literal");
        }

        advance(); // Skip closing quote
        return result;
    }

    void Lexer::skipWhitespaceAndComments()
    {
        while (pos < input.length())
        {
            char current = input[pos];

            // Skip whitespace
            if (std::isspace(current))
            {
                advance();
                continue;
            }

            // Skip single-line comments
            if (current == '/' && pos + 1 < input.length() && input[pos + 1] == '/')
            {
                while (pos < input.length() && input[pos] != '\n')
                {
                    advance();
                }
                continue;
            }

            // Skip multi-line comments
            if (current == '/' && pos + 1 < input.length() && input[pos + 1] == '*')
            {
                advance(); // Skip '/'
                advance(); // Skip '*'
                while (pos + 1 < input.length() && !(input[pos] == '*' && input[pos + 1] == '/'))
                {
                    advance();
                }
                if (pos + 1 < input.length())
                {
                    advance(); // Skip '*'
                    advance(); // Skip '/'
                }
                continue;
            }

            break;
        }
    }

    void Lexer::advance()
    {
        if (pos < input.length())
        {
            if (input[pos] == '\n')
            {
                currentLine++;
                currentColumn = 1;
            }
            else
            {
                currentColumn++;
            }
            pos++;
        }
    }

    void Lexer::throwError(const std::string& message)
    {
        throw errors::ParseException(message, errors::SourceLocation(filename, currentLine, currentColumn));
    }
}
