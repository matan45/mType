#include "Lexer.hpp"
#include <cctype>
#include <stdexcept>
#include <limits>
#include <iostream>
#include "TokenFactory.hpp"
#include "../errors/ParseException.hpp"

namespace lexer
{
    // Static operator lookup table definitions
    const std::array<Lexer::OperatorInfo, 14> Lexer::TWO_CHAR_OPERATORS = {{
        {"++", TokenType::INCREMENT, 2},
        {"--", TokenType::DECREMENT, 2},
        {"==", TokenType::EQUALS, 2},
        {"!=", TokenType::NOT_EQUALS, 2},
        {"<=", TokenType::LESS_EQUALS, 2},
        {">=", TokenType::GREATER_EQUALS, 2},
        {"&&", TokenType::AND, 2},
        {"||", TokenType::OR, 2},
        {"+=", TokenType::PLUS_ASSIGN, 2},
        {"-=", TokenType::MINUS_ASSIGN, 2},
        {"*=", TokenType::MULTIPLY_ASSIGN, 2},
        {"/=", TokenType::DIVIDE_ASSIGN, 2},
        {"%=", TokenType::MODULO_ASSIGN, 2},
        {"::", TokenType::SCOPE, 2}
    }};

    const std::array<Lexer::OperatorInfo, 20> Lexer::SINGLE_CHAR_OPERATORS = {{
        {"+", TokenType::PLUS, 1},
        {"-", TokenType::MINUS, 1},
        {"*", TokenType::MULTIPLY, 1},
        {"/", TokenType::DIVIDE, 1},
        {"%", TokenType::MODULO, 1},
        {"=", TokenType::ASSIGN, 1},
        {"(", TokenType::LPAREN, 1},
        {")", TokenType::RPAREN, 1},
        {"{", TokenType::LBRACE, 1},
        {"}", TokenType::RBRACE, 1},
        {"[", TokenType::LBRACKET, 1},
        {"]", TokenType::RBRACKET, 1},
        {",", TokenType::COMMA, 1},
        {";", TokenType::SEMICOLON, 1},
        {":", TokenType::COLON, 1},
        {"?", TokenType::QUESTION, 1},
        {"!", TokenType::NOT, 1},
        {"<", TokenType::LESS, 1},
        {">", TokenType::GREATER, 1},
        {".", TokenType::DOT, 1}
    }};

    Lexer::Lexer(const std::string& filePath, std::unique_ptr<FileReader> reader)
        : pos(0), fileReader(std::move(reader)),
          locationTracker(std::make_unique<SourceLocationTracker>(filePath)),
          bracketBalancer(std::make_unique<BracketBalancer>())
    {
        input = fileReader->readFile(filePath);
        locationTracker->splitIntoLines(input);
    }

    Token Lexer::getNextToken()
    {
        skipWhitespaceAndComments();

        if (pos >= input.length())
        {
            return TokenFactory::createEndToken(locationTracker->getCurrentLocation());
        }

        char current = input[pos];
        errors::SourceLocation location = locationTracker->getCurrentLocation();

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
                return TokenFactory::createFloatToken(value, location);
            }
            else
            {
                int value = parseInteger();
                return TokenFactory::createIntegerToken(value, location);
            }
        }

        // Identifiers and keywords
        if (std::isalpha(current) || current == '_')
        {
            std::string_view identifier = parseIdentifier();
            TokenType tokenType = findKeywordType(identifier);
            
            if (tokenType == TokenType::IDENTIFIER)
            {
                return TokenFactory::createIdentifierToken(identifier, location);
            }
            else
            {
                return TokenFactory::createKeywordToken(tokenType, identifier, location);
            }
        }

        // String literals
        if (current == '"')
        {
            std::string value = parseStringLiteral();
            return TokenFactory::createStringToken(value, location);
        }

        // Try to parse operators using lookup tables
        Token operatorToken = tryParseOperator();
        if (operatorToken.type != TokenType::END) // Check if a valid operator was found
        {
            return operatorToken;
        }

        // If no operator was found, handle unexpected character
        throwError("Unexpected character: " + std::string(1, current));
    }

    Token Lexer::peekNextToken()
    {
        // Save current state
        size_t savedPos = pos;
        int savedLine = locationTracker->getCurrentLine();
        int savedColumn = locationTracker->getCurrentColumn();
        std::stack<char> savedBalanceStack = bracketBalancer->copyStack();

        // Get the next token
        Token token = getNextToken();

        // Restore state
        pos = savedPos;
        locationTracker->setPosition(savedLine, savedColumn);
        
        // Restore balance stack
        bracketBalancer->clear();
        std::stack<char> tempStack = savedBalanceStack;
        std::vector<char> stackContents;
        
        // Extract all elements from the stack
        while (!tempStack.empty())
        {
            stackContents.push_back(tempStack.top());
            tempStack.pop();
        }
        
        // Push them back in reverse order to restore original stack
        for (auto it = stackContents.rbegin(); it != stackContents.rend(); ++it)
        {
            bracketBalancer->pushOpening(*it);
        }

        return token;
    }


    float Lexer::parseFloat()
    {
        size_t start = pos;
        while (pos < input.length() && (std::isdigit(input[pos]) || input[pos] == '.'))
        {
            advance();
        }
        std::string_view floatView(input.data() + start, pos - start);
        try
        {
            return std::stof(std::string(floatView)); // std::stof requires std::string
        }catch (const std::out_of_range&)
        {
            // Handle float overflow - clamp to float limits
            std::string floatStr(floatView); // Convert to string for error handling
            long double value;
            try
            {
                value = std::stold(floatStr);
            }
            catch (const std::out_of_range&)
            {
                // If even long double overflows, return max/min float
                return (floatView[0] == '-') ? std::numeric_limits<float>::lowest() : std::numeric_limits<float>::max();
            }

            // Clamp to float range
            if (value > std::numeric_limits<float>::max()) return std::numeric_limits<float>::max();
            if (value < std::numeric_limits<float>::lowest()) return std::numeric_limits<float>::lowest();
            return static_cast<float>(value);
        }
        catch (const std::invalid_argument&)
        {
            throw errors::ParseException("Invalid float format: " + std::string(floatView), locationTracker->getCurrentLocation());
        }
        
    }

    int Lexer::parseInteger()
    {
        size_t start = pos;
        while (pos < input.length() && std::isdigit(input[pos]))
        {
            advance();
        }
        std::string_view intView(input.data() + start, pos - start);

        try
        {
            return std::stoi(std::string(intView));
        }
        catch (const std::out_of_range&)
        {
            // Handle integer overflow - clamp to int limits
            std::string intStr(intView); // Convert to string for error handling
            long long value;
            try
            {
                value = std::stoll(intStr);
            }
            catch (const std::out_of_range&)
            {
                // If even long overflows, return max/min int
                return (intView[0] == '-') ? INT_MIN : INT_MAX;
            }

            // Clamp to int range
            if (value > INT_MAX) return INT_MAX;
            if (value < INT_MIN) return INT_MIN;
            return static_cast<int>(value);
        }
        catch (const std::invalid_argument&)
        {
            throw errors::ParseException("Invalid integer format: " + std::string(intView), locationTracker->getCurrentLocation());
        }
    }

    std::string_view Lexer::parseIdentifier()
    {
        size_t start = pos;
        while (pos < input.length() && (std::isalnum(input[pos]) || input[pos] == '_'))
        {
            advance();
        }
        return std::string_view(input.data() + start, pos - start);
    }

    std::string Lexer::parseStringLiteral()
    {
        advance(); // Skip opening quote
        size_t start = pos;
        
        // First pass: find end and check if we have escape sequences
        bool hasEscapes = false;
        size_t end = pos;
        while (end < input.length() && input[end] != '"')
        {
            if (input[end] == '\\')
            {
                hasEscapes = true;
                if (end + 1 < input.length()) end++; // Skip escaped character
            }
            end++;
        }
        
        if (end >= input.length())
        {
            throwError("Unterminated string literal");
        }
        
        std::string result;
        if (!hasEscapes)
        {
            // Optimization: no escapes, use string_view for direct copy
            std::string_view literalView(input.data() + start, end - start);
            result = std::string(literalView);
            pos = end + 1; // Skip closing quote
        }
        else
        {
            // Handle escape sequences (original logic)
            result.reserve(end - start); // Pre-allocate approximate size
            while (pos < input.length() && input[pos] != '"')
            {
                if (input[pos] == '\\' && pos + 1 < input.length())
                {
                    advance(); // Skip backslash
                    switch (input[pos])
                    {
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    case 'r': result += '\r'; break;
                    case '\\': result += '\\'; break;
                    case '"': result += '"'; break;
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
            advance(); // Skip closing quote
        }

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
            locationTracker->advance(input[pos]);
            pos++;
        }
    }

    void Lexer::throwError(const std::string& message)
    {
        throw errors::ParseException(message, locationTracker->getCurrentLocation());
    }

    Token Lexer::tryParseOperator()
    {
        errors::SourceLocation location = locationTracker->getCurrentLocation();

        // Try two-character operators first
        if (pos + 1 < input.length())
        {
            std::string_view twoChar(input.data() + pos, 2);
            for (const auto& op : TWO_CHAR_OPERATORS)
            {
                if (twoChar == op.symbol)
                {
                    advanceMultiple(op.length);
                    return TokenFactory::createOperatorToken(op.type, op.symbol, location);
                }
            }
        }

        // Before trying single-character operators, check for spaced operators
        // that might be tokenized incorrectly as single characters
        if (pos < input.length())
        {
            char current = input[pos];
            if (current == '=' || current == '!' || current == '<' || current == '>' ||
                current == '+' || current == '-' || current == '*' || current == '/' ||
                current == '%' || current == '&' || current == '|')
            {
                Token spacedToken = tryParseSpacedOperator();
                if (spacedToken.type != TokenType::END)
                {
                    return spacedToken;
                }
            }
        }

        // Try single-character operators
        if (pos < input.length())
        {
            std::string_view singleChar(input.data() + pos, 1);
            for (const auto& op : SINGLE_CHAR_OPERATORS)
            {
                if (singleChar == op.symbol)
                {
                    // Handle bracket balancing logic
                    char current = input[pos];
                    if (bracketBalancer->isOpeningBracket(current))
                    {
                        bracketBalancer->pushOpening(current);
                    }
                    else if (bracketBalancer->isClosingBracket(current))
                    {
                        bracketBalancer->validateClosing(current, location);
                    }

                    advanceMultiple(op.length);
                    return TokenFactory::createOperatorToken(op.type, op.symbol, location);
                }
            }
        }

        // No operator found
        return TokenFactory::createEndToken(location); // Invalid token as sentinel
    }

    Token Lexer::tryParseSpacedOperator()
    {
        errors::SourceLocation location = locationTracker->getCurrentLocation();
        size_t originalPos = pos;

        // Look for spaced two-character operators
        if (pos < input.length())
        {
            char firstChar = input[pos];
            size_t tempPos = pos + 1;

            // Skip whitespace
            while (tempPos < input.length() && std::isspace(input[tempPos]))
            {
                tempPos++;
            }

            // Check if we have a second character that forms a valid spaced operator
            if (tempPos < input.length())
            {
                char secondChar = input[tempPos];

                // Check all possible spaced operator combinations
                TokenType operatorType = TokenType::END;
                std::string_view operatorSymbol;

                // Comparison operators
                if (firstChar == '=' && secondChar == '=')
                {
                    operatorType = TokenType::EQUALS;
                    operatorSymbol = "==";
                }
                else if (firstChar == '!' && secondChar == '=')
                {
                    operatorType = TokenType::NOT_EQUALS;
                    operatorSymbol = "!=";
                }
                else if (firstChar == '<' && secondChar == '=')
                {
                    operatorType = TokenType::LESS_EQUALS;
                    operatorSymbol = "<=";
                }
                else if (firstChar == '>' && secondChar == '=')
                {
                    operatorType = TokenType::GREATER_EQUALS;
                    operatorSymbol = ">=";
                }
                // Increment/Decrement operators
                else if (firstChar == '+' && secondChar == '+')
                {
                    operatorType = TokenType::INCREMENT;
                    operatorSymbol = "++";
                }
                else if (firstChar == '-' && secondChar == '-')
                {
                    operatorType = TokenType::DECREMENT;
                    operatorSymbol = "--";
                }
                // Logical operators
                else if (firstChar == '&' && secondChar == '&')
                {
                    operatorType = TokenType::AND;
                    operatorSymbol = "&&";
                }
                else if (firstChar == '|' && secondChar == '|')
                {
                    operatorType = TokenType::OR;
                    operatorSymbol = "||";
                }
                // Assignment operators
                else if (firstChar == '+' && secondChar == '=')
                {
                    operatorType = TokenType::PLUS_ASSIGN;
                    operatorSymbol = "+=";
                }
                else if (firstChar == '-' && secondChar == '=')
                {
                    operatorType = TokenType::MINUS_ASSIGN;
                    operatorSymbol = "-=";
                }
                else if (firstChar == '*' && secondChar == '=')
                {
                    operatorType = TokenType::MULTIPLY_ASSIGN;
                    operatorSymbol = "*=";
                }
                else if (firstChar == '/' && secondChar == '=')
                {
                    operatorType = TokenType::DIVIDE_ASSIGN;
                    operatorSymbol = "/=";
                }
                else if (firstChar == '%' && secondChar == '=')
                {
                    operatorType = TokenType::MODULO_ASSIGN;
                    operatorSymbol = "%=";
                }

                if (operatorType != TokenType::END)
                {
                    // Update position to after the second character
                    pos = tempPos + 1;
                    return TokenFactory::createOperatorToken(operatorType, operatorSymbol, location);
                }
            }
        }

        // No spaced operator found, restore position
        pos = originalPos;
        return TokenFactory::createEndToken(location);
    }


    void Lexer::advanceMultiple(size_t count)
    {
        for (size_t i = 0; i < count && pos < input.length(); ++i)
        {
            advance();
        }
    }

    TokenType Lexer::findKeywordType(std::string_view identifier) const
    {
        // Use map lookup for all keywords to ensure correctness
        // The optimization can be added later if needed, but correctness first
        auto keywordIt = keywords.find(std::string(identifier));
        if (keywordIt != keywords.end()) 
        {
            return keywordIt->second;
        }
        
        return TokenType::IDENTIFIER; // Not a keyword
    }

}
