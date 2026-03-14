#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <array>
#include <string_view>
#include <vector>
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

        // Interpolation state tracking
        struct InterpolationState
        {
            bool active = false;    // currently inside interpolated string
            int braceDepth = 0;     // tracks nested {} inside expressions
        };
        InterpolationState interpolationState;

        // Helper struct for state save/restore during peeking operations
        struct LexerState
        {
            size_t pos;
            int line;
            int column;
            std::stack<char> balanceStack;
            InterpolationState interpState;
        };

        // Note: Deep lookahead implemented via position save/restore
        // More complex buffering could be added later for performance

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
        static const std::array<OperatorInfo, 17> TWO_CHAR_OPERATORS;
        static const std::array<OperatorInfo, 25> SINGLE_CHAR_OPERATORS;

        // Static keyword lookup table
        static const std::unordered_map<std::string, TokenType> keywords;

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

        // Deep lookahead support for complex parsing scenarios
        Token peekAhead(size_t offset);
        std::vector<Token> peekMultiple(size_t count);

    private:
        // Core parsing methods
        Token parseNumber();
        double parseFloat();
        int64_t parseInteger();
        std::string_view parseIdentifier();
        std::string parseStringLiteral();
        std::string processEscapeSequences(size_t start, size_t end);
        bool processEscapeChar(char escaped, std::string& out);
        Token parseInterpolatedString();
        Token scanInterpolatedSegment();
        void skipWhitespaceAndComments();

        // Movement and positioning
        void advance();
        void advanceMultiple(size_t count);

        // Boundary checking helpers
        bool canPeekAhead(size_t offset = 1) const;
        char peekChar(size_t offset = 0) const;

        // Token creation helpers
        Token tryParseOperator();
        Token tryParseSpacedOperator();
        TokenType findKeywordType(std::string_view identifier) const;

        // State management for peeking
        LexerState captureState() const;
        void restoreState(const LexerState& state);

        // Error handling
        [[noreturn]] void throwError(const std::string& message);
    };
}
