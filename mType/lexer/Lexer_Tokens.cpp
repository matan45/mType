#include "Lexer.hpp"
#include <cstddef>
#include <cstdint>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <cerrno>
#include <system_error>
#include <limits>
#include "TokenFactory.hpp"
#include "../errors/ParseException.hpp"
#include "../constants/SecurityConstants.hpp"

namespace lexer
{
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

    Token Lexer::parseNumber()
    {
        errors::SourceLocation location = locationTracker->getCurrentLocation();

        size_t lookAhead = pos;
        while (lookAhead < input.length() && std::isdigit(input[lookAhead]))
        {
            lookAhead++;
        }

        // Decimal point followed by digits ⇒ float; otherwise integer.
        if (lookAhead < input.length() && input[lookAhead] == '.' &&
            lookAhead + 1 < input.length() && std::isdigit(input[lookAhead + 1]))
        {
            double value = parseFloat();
            return TokenFactory::createFloatToken(value, location);
        }
        int64_t value = parseInteger();
        return TokenFactory::createIntegerToken(value, location);
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
                advance();
                processEscapeChar(input[pos], result);
            }
            else
            {
                result += input[pos];
            }
            advance();
        }
        advance(); // skip closing quote

        return storeProcessed(std::move(result));
    }

    std::string_view Lexer::parseStringLiteral()
    {
        advance(); // skip opening quote
        size_t start = pos;

        // First pass: find end and check if we have escape sequences.
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
                if (end + 1 < input.length()) end++; // skip escaped character
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
            // MYT-307: sync locationTracker across the literal body and
            // closing quote. Skipping advance() here would freeze
            // currentColumn after the opening quote and silently corrupt
            // the column of every subsequent token on this line.
            while (pos < end) advance();
            advance(); // skip closing quote
            return literalView;
        }

        return processEscapeSequences(start, end);
    }

    TokenType Lexer::findKeywordType(std::string_view identifier) const
    {
        auto keywordIt = keywords.find(std::string(identifier));
        if (keywordIt != keywords.end())
        {
            return keywordIt->second;
        }
        return TokenType::IDENTIFIER;
    }
}
