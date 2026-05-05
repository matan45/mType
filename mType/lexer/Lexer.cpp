#include "Lexer.hpp"
#include <cstddef>
#include <cstdint>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <limits>
#include "TokenFactory.hpp"
#include "../errors/ParseException.hpp"
#include "../constants/SecurityConstants.hpp"
#include "../diagnostics/SourceFileCache.hpp"

namespace lexer
{
    // Static operator lookup table definitions
    const std::array<Lexer::OperatorInfo, 17> Lexer::TWO_CHAR_OPERATORS = {
        {
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
            {"::", TokenType::SCOPE, 2},
            {"->", TokenType::ARROW, 2},
            {"<<", TokenType::LEFT_SHIFT, 2},
            {">>", TokenType::RIGHT_SHIFT, 2}
        }
    };

    const std::array<Lexer::OperatorInfo, 25> Lexer::SINGLE_CHAR_OPERATORS = {
        {
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
            {".", TokenType::DOT, 1},
            {"@", TokenType::AT, 1},
            {"&", TokenType::BITWISE_AND, 1},
            {"|", TokenType::BITWISE_OR, 1},
            {"^", TokenType::BITWISE_XOR, 1},
            {"~", TokenType::BITWISE_NOT, 1}
        }
    };

    const std::unordered_map<std::string, TokenType> Lexer::keywords = {
        {"function", TokenType::FUNCTION},
        {"return", TokenType::RETURN},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"do", TokenType::DO},
        {"for", TokenType::FOR},
        {"final", TokenType::FINAL},
        {"abstract", TokenType::ABSTRACT},
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
        {"private", TokenType::PRIVATE},
        {"public", TokenType::PUBLIC},
        {"protected", TokenType::PROTECTED},
        {"constructor", TokenType::CONSTRUCTOR},
        {"null", TokenType::NULL_LITERAL},
        {"true", TokenType::TRUE},
        {"import", TokenType::IMPORT},
        {"from", TokenType::FROM},
        {"false", TokenType::FALSE},
        {"switch", TokenType::SWITCH},
        {"case", TokenType::CASE},
        {"match", TokenType::MATCH},
        {"interface", TokenType::INTERFACE},
        {"implements", TokenType::IMPLEMENTS},
        {"extends", TokenType::EXTENDS},
        {"super", TokenType::SUPER},
        {"default", TokenType::DEFAULT},
        {"isClassOf", TokenType::ISCLASSOF},
        {"async", TokenType::ASYNC},
        {"await", TokenType::AWAIT},
        {"try", TokenType::TRY},
        {"catch", TokenType::CATCH},
        {"throw", TokenType::THROW},
        {"finally", TokenType::FINALLY},
        {"annotation", TokenType::ANNOTATION}
    };

    Lexer::Lexer(const std::string& filePath, std::unique_ptr<FileReader> reader)
        : pos(0), fileReader(std::move(reader)),
          locationTracker(std::make_unique<SourceLocationTracker>(filePath)),
          bracketBalancer(std::make_unique<BracketBalancer>())
    {
        input = fileReader->readFile(filePath);
        locationTracker->splitIntoLines(input);

        // Publish source lines so the diagnostic renderer can produce
        // snippets without re-reading the file from disk.
        diagnostics::SourceFileCache::instance().publish(
            filePath, locationTracker->getLines());
    }

    Token Lexer::parseNumber()
    {
        errors::SourceLocation location = locationTracker->getCurrentLocation();

        // Look ahead to determine if this is a float or integer
        size_t lookAhead = pos;
        while (lookAhead < input.length() && std::isdigit(input[lookAhead]))
        {
            lookAhead++;
        }

        // Check if we found a decimal point followed by digits
        if (lookAhead < input.length() && input[lookAhead] == '.' &&
            lookAhead + 1 < input.length() && std::isdigit(input[lookAhead + 1]))
        {
            double value = parseFloat();
            return TokenFactory::createFloatToken(value, location);
        }
        else
        {
            int64_t value = parseInteger();
            return TokenFactory::createIntegerToken(value, location);
        }
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

        // Interpolation state: when expression ends with }, resume string scanning
        if (interpolationState.active && current == '}' && interpolationState.braceDepth == 0)
        {
            advance(); // consume '}'
            return scanInterpolatedSegment();
        }

        // Track nested braces inside interpolation expressions
        if (interpolationState.active && current == '{')
        {
            interpolationState.braceDepth++;
        }
        if (interpolationState.active && current == '}' && interpolationState.braceDepth > 0)
        {
            interpolationState.braceDepth--;
        }

        // Numbers
        if (std::isdigit(current))
        {
            return parseNumber();
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

        // Interpolated string literals: $"..."
        if (current == '$' && pos + 1 < input.length() && input[pos + 1] == '"')
        {
            return parseInterpolatedString();
        }

        // String literals
        if (current == '"')
        {
            std::string_view value = parseStringLiteral();
            return TokenFactory::createStringToken(value, location);
        }

        // Try to parse operators using lookup tables
        Token operatorToken = tryParseOperator();
        if (operatorToken.type != TokenType::END)
        {
            return operatorToken;
        }

        // If no operator was found, handle unexpected character
        throwError("Unexpected character: " + std::string(1, current));
    }

    Lexer::LexerState Lexer::captureState() const
    {
        return LexerState{
            pos,
            locationTracker->getCurrentLine(),
            locationTracker->getCurrentColumn(),
            bracketBalancer->copyStack(),
            interpolationState
        };
    }

    void Lexer::restoreState(const LexerState& state)
    {
        pos = state.pos;
        locationTracker->setPosition(state.line, state.column);
        interpolationState = state.interpState;

        // Restore bracket balancer stack
        bracketBalancer->clear();
        std::stack<char> tempStack = state.balanceStack;
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
    }

    Token Lexer::peekNextToken()
    {
        LexerState savedState = captureState();
        Token token = getNextToken();
        restoreState(savedState);
        return token;
    }


    double Lexer::parseFloat()
    {
        size_t start = pos;
        while (pos < input.length() && (std::isdigit(input[pos]) || input[pos] == '.'))
        {
            // SECURITY: cap numeric literal length to prevent oversized
            // tokens from exhausting memory.
            if (pos - start >= constants::security::MAX_NUMBER_LITERAL_LENGTH)
            {
                throw errors::ParseException(
                    "Float literal exceeds maximum length of " +
                    std::to_string(constants::security::MAX_NUMBER_LITERAL_LENGTH) + " characters",
                    locationTracker->getCurrentLocation());
            }
            advance();
        }
        const std::string_view floatView(input.data() + start, pos - start);

        double value = 0.0;
        bool outOfRange = false;
        bool invalid    = false;

#if defined(_LIBCPP_VERSION)
        // libc++ (Apple/Xcode) does not yet implement std::from_chars for
        // floating-point types; fall back to strtod with a null-terminated copy.
        const std::string buf(floatView);
        errno = 0;
        char* endPtr = nullptr;
        value      = std::strtod(buf.c_str(), &endPtr);
        outOfRange = (errno == ERANGE);
        invalid    = (endPtr == buf.c_str() || endPtr != buf.c_str() + buf.size());
#else
        const char* first = floatView.data();
        const char* last  = floatView.data() + floatView.size();
        const auto res    = std::from_chars(first, last, value);
        outOfRange = (res.ec == std::errc::result_out_of_range);
        invalid    = (res.ec == std::errc::invalid_argument || res.ptr != last);
#endif

        if (outOfRange)
        {
            throw errors::ParseException(
                "Float literal '" + std::string(floatView) + "' is out of range for type 'float'",
                locationTracker->getCurrentLocation());
        }
        if (invalid)
        {
            throw errors::ParseException("Invalid float format: " + std::string(floatView),
                                         locationTracker->getCurrentLocation());
        }
        return value;
    }

    int64_t Lexer::parseInteger()
    {
        size_t start = pos;
        while (pos < input.length() && std::isdigit(input[pos]))
        {
            // SECURITY: cap numeric literal length.
            if (pos - start >= constants::security::MAX_NUMBER_LITERAL_LENGTH)
            {
                throw errors::ParseException(
                    "Integer literal exceeds maximum length of " +
                    std::to_string(constants::security::MAX_NUMBER_LITERAL_LENGTH) + " characters",
                    locationTracker->getCurrentLocation());
            }
            advance();
        }
        const std::string_view intView(input.data() + start, pos - start);

        int64_t value = 0;
        const char* first = intView.data();
        const char* last  = intView.data() + intView.size();
        const auto res = std::from_chars(first, last, value, 10);

        if (res.ec == std::errc::result_out_of_range)
        {
            throw errors::ParseException(
                "Integer literal '" + std::string(intView) + "' is out of range for type 'int' (must be between " +
                std::to_string(LLONG_MIN) + " and " + std::to_string(LLONG_MAX) + ")",
                locationTracker->getCurrentLocation());
        }
        if (res.ec == std::errc::invalid_argument || res.ptr != last)
        {
            throw errors::ParseException("Invalid integer format: " + std::string(intView),
                                         locationTracker->getCurrentLocation());
        }
        return value;
    }

    std::string_view Lexer::parseIdentifier()
    {
        size_t start = pos;
        while (pos < input.length() && (std::isalnum(input[pos]) || input[pos] == '_'))
        {
            // SECURITY: cap identifier length to prevent multi-megabyte
            // identifiers from a crafted source file.
            if (pos - start >= constants::security::MAX_IDENTIFIER_LENGTH)
            {
                throw errors::ParseException(
                    "Identifier exceeds maximum length of " +
                    std::to_string(constants::security::MAX_IDENTIFIER_LENGTH) + " characters",
                    locationTracker->getCurrentLocation());
            }
            advance();
        }
        return std::string_view(input.data() + start, pos - start);
    }

    bool Lexer::processEscapeChar(char escaped, std::string& out)
    {
        switch (escaped)
        {
        case 'n': out += '\n'; return true;
        case 't': out += '\t'; return true;
        case 'r': out += '\r'; return true;
        case '\\': out += '\\'; return true;
        case '"': out += '"'; return true;
        case '{': out += '{'; return true;
        case '}': out += '}'; return true;
        default:
            out += '\\';
            out += escaped;
            return true;
        }
    }

    std::string_view Lexer::storeProcessed(std::string&& s)
    {
        processedLiteralArena.emplace_back(std::move(s));
        return processedLiteralArena.back();
    }

    std::string_view Lexer::processEscapeSequences(size_t start, size_t end)
    {
        std::string result;
        result.reserve(end - start);

        while (pos < input.length() && input[pos] != '"')
        {
            // SECURITY: cap accumulated string length so a 1 GB literal
            // cannot be assembled from many short escape sequences.
            if (result.size() >= constants::security::MAX_STRING_LITERAL_LENGTH)
            {
                throw errors::ParseException(
                    "String literal exceeds maximum length of " +
                    std::to_string(constants::security::MAX_STRING_LITERAL_LENGTH) + " characters",
                    locationTracker->getCurrentLocation());
            }
            if (input[pos] == '\\' && pos + 1 < input.length())
            {
                advance(); // Skip backslash
                processEscapeChar(input[pos], result);
            }
            else
            {
                result += input[pos];
            }
            advance();
        }
        advance(); // Skip closing quote

        return storeProcessed(std::move(result));
    }

    std::string_view Lexer::parseStringLiteral()
    {
        advance(); // Skip opening quote
        size_t start = pos;

        // First pass: find end and check if we have escape sequences
        bool hasEscapes = false;
        size_t end = pos;
        while (end < input.length() && input[end] != '"')
        {
            // SECURITY: cap raw string literal length on the size-finding
            // pass too, before we ever allocate.
            if (end - start >= constants::security::MAX_STRING_LITERAL_LENGTH)
            {
                throw errors::ParseException(
                    "String literal exceeds maximum length of " +
                    std::to_string(constants::security::MAX_STRING_LITERAL_LENGTH) + " characters",
                    locationTracker->getCurrentLocation());
            }
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

        if (!hasEscapes)
        {
            // Fast path: view directly into the source buffer (stable for
            // Lexer lifetime). No allocation.
            std::string_view literalView(input.data() + start, end - start);
            pos = end + 1; // Skip closing quote
            return literalView;
        }

        return processEscapeSequences(start, end);
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
        auto location = locationTracker->getCurrentLocation();
        throw errors::ParseException(message, location);
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

        if (pos >= input.length())
        {
            return TokenFactory::createEndToken(location);
        }

        char firstChar = input[pos];
        size_t tempPos = pos + 1;

        // Skip whitespace between operator characters
        while (tempPos < input.length() && std::isspace(input[tempPos]))
        {
            tempPos++;
        }

        if (tempPos >= input.length())
        {
            return TokenFactory::createEndToken(location);
        }

        char secondChar = input[tempPos];

        // Compare characters directly against operator table
        // SAFETY: Direct character comparison avoids temporary buffer allocation
        // and eliminates any potential string_view lifetime issues
        for (const auto& op : TWO_CHAR_OPERATORS)
        {
            if (op.symbol.length() == 2 &&
                op.symbol[0] == firstChar &&
                op.symbol[1] == secondChar)
            {
                // Found a valid spaced operator - advance past both characters
                pos = tempPos + 1;
                return TokenFactory::createOperatorToken(op.type, op.symbol, location);
            }
        }

        // No spaced operator found
        return TokenFactory::createEndToken(location);
    }


    Token Lexer::parseInterpolatedString()
    {
        errors::SourceLocation location = locationTracker->getCurrentLocation();
        advance(); // skip '$'
        advance(); // skip '"'

        // Scan text until '{' or '"'
        std::string text;
        while (pos < input.length() && input[pos] != '"' && input[pos] != '{')
        {
            if (input[pos] == '\\' && pos + 1 < input.length())
            {
                advance(); // skip backslash
                processEscapeChar(input[pos], text);
                advance();
                continue;
            }
            text += input[pos];
            advance();
        }

        if (pos >= input.length())
        {
            throwError("Unterminated interpolated string literal");
        }

        if (input[pos] == '"')
        {
            // No interpolation found, treat as regular string.
            // `text` is a local std::string (escapes already processed); the
            // token needs a string_view that outlives this function, so move
            // text into the arena.
            advance(); // skip closing '"'
            return TokenFactory::createStringToken(storeProcessed(std::move(text)), location);
        }

        // Found '{' - emit INTERP_STRING_BEGIN
        advance(); // skip '{'
        interpolationState.active = true;
        interpolationState.braceDepth = 0;
        return Token{TokenType::INTERP_STRING_BEGIN, 0.0, 0,
                     storeProcessed(std::move(text)), location};
    }

    Token Lexer::scanInterpolatedSegment()
    {
        errors::SourceLocation location = locationTracker->getCurrentLocation();
        std::string text;

        while (pos < input.length() && input[pos] != '"' && input[pos] != '{')
        {
            if (input[pos] == '\\' && pos + 1 < input.length())
            {
                advance(); // skip backslash
                processEscapeChar(input[pos], text);
                advance();
                continue;
            }
            text += input[pos];
            advance();
        }

        if (pos >= input.length())
        {
            throwError("Unterminated interpolated string literal");
        }

        if (input[pos] == '{')
        {
            // More expressions to come
            advance(); // skip '{'
            interpolationState.braceDepth = 0;
            return Token{TokenType::INTERP_STRING_MIDDLE, 0.0, 0,
                         storeProcessed(std::move(text)), location};
        }

        // Found closing '"'
        advance(); // skip '"'
        interpolationState.active = false;
        return Token{TokenType::INTERP_STRING_END, 0.0, 0,
                     storeProcessed(std::move(text)), location};
    }

    void Lexer::advanceMultiple(size_t count)
    {
        for (size_t i = 0; i < count && pos < input.length(); ++i)
        {
            advance();
        }
    }

    bool Lexer::canPeekAhead(size_t offset) const
    {
        return pos + offset < input.length();
    }

    char Lexer::peekChar(size_t offset) const
    {
        if (canPeekAhead(offset))
        {
            return input[pos + offset];
        }
        return '\0';
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

    Token Lexer::peekAhead(size_t offset)
    {
        if (offset == 0)
        {
            return peekNextToken();
        }

        LexerState savedState = captureState();

        // Advance to the target token
        Token result;
        for (size_t i = 0; i < offset; ++i)
        {
            result = getNextToken();

            // Check if we hit end of input
            if (result.type == TokenType::END)
            {
                break;
            }
        }

        restoreState(savedState);
        return result;
    }

    std::vector<Token> Lexer::peekMultiple(size_t count)
    {
        LexerState savedState = captureState();

        std::vector<Token> tokens;
        tokens.reserve(count);

        for (size_t i = 0; i < count; ++i)
        {
            Token token = getNextToken();
            tokens.push_back(token);

            if (token.type == TokenType::END)
            {
                break;
            }
        }

        restoreState(savedState);
        return tokens;
    }
}
