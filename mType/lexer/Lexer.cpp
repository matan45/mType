#include "Lexer.hpp"
#include <cstddef>
#include <cctype>
#include "TokenFactory.hpp"
#include "../errors/ParseException.hpp"
#include "../diagnostics/SourceFileCache.hpp"

namespace lexer
{
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

    Token Lexer::getNextToken()
    {
        skipWhitespaceAndComments();

        if (pos >= input.length())
        {
            return TokenFactory::createEndToken(locationTracker->getCurrentLocation());
        }

        char current = input[pos];
        errors::SourceLocation location = locationTracker->getCurrentLocation();

        // Interpolation: when the expression closes with `}`, resume string scanning.
        if (interpolationState.active && current == '}' && interpolationState.braceDepth == 0)
        {
            advance();
            return scanInterpolatedSegment();
        }

        // Track nested braces inside interpolation expressions.
        if (interpolationState.active && current == '{')
        {
            interpolationState.braceDepth++;
        }
        if (interpolationState.active && current == '}' && interpolationState.braceDepth > 0)
        {
            interpolationState.braceDepth--;
        }

        if (std::isdigit(current))
        {
            return parseNumber();
        }

        if (std::isalpha(current) || current == '_')
        {
            std::string_view identifier = parseIdentifier();
            TokenType tokenType = findKeywordType(identifier);

            if (tokenType == TokenType::IDENTIFIER)
            {
                return TokenFactory::createIdentifierToken(identifier, location);
            }
            return TokenFactory::createKeywordToken(tokenType, identifier, location);
        }

        // Interpolated string literals: $"..."
        if (current == '$' && pos + 1 < input.length() && input[pos + 1] == '"')
        {
            return parseInterpolatedString();
        }

        if (current == '"')
        {
            std::string_view value = parseStringLiteral();
            return TokenFactory::createStringToken(value, location);
        }

        Token operatorToken = tryParseOperator();
        if (operatorToken.type != TokenType::END)
        {
            return operatorToken;
        }

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

        // Drain the saved stack into a vector, then push back in reverse so
        // the balancer's stack matches the captured top-to-bottom order.
        bracketBalancer->clear();
        std::stack<char> tempStack = state.balanceStack;
        std::vector<char> stackContents;

        while (!tempStack.empty())
        {
            stackContents.push_back(tempStack.top());
            tempStack.pop();
        }

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

    void Lexer::skipWhitespaceAndComments()
    {
        while (pos < input.length())
        {
            char current = input[pos];

            if (std::isspace(current))
            {
                advance();
                continue;
            }

            // Single-line comments: // until newline.
            if (current == '/' && pos + 1 < input.length() && input[pos + 1] == '/')
            {
                while (pos < input.length() && input[pos] != '\n')
                {
                    advance();
                }
                continue;
            }

            // Multi-line comments: /* ... */.
            if (current == '/' && pos + 1 < input.length() && input[pos + 1] == '*')
            {
                advance();
                advance();
                while (pos + 1 < input.length() && !(input[pos] == '*' && input[pos + 1] == '/'))
                {
                    advance();
                }
                if (pos + 1 < input.length())
                {
                    advance();
                    advance();
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

        // Two-character operators with whitespace between the chars (`= =`, `! =`).
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

        if (pos < input.length())
        {
            std::string_view singleChar(input.data() + pos, 1);
            for (const auto& op : SINGLE_CHAR_OPERATORS)
            {
                if (singleChar == op.symbol)
                {
                    // Bracket balancing piggybacks on single-char operator emit.
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

        // Sentinel: END means "no operator matched".
        return TokenFactory::createEndToken(location);
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

        while (tempPos < input.length() && std::isspace(input[tempPos]))
        {
            tempPos++;
        }

        if (tempPos >= input.length())
        {
            return TokenFactory::createEndToken(location);
        }

        char secondChar = input[tempPos];

        // SAFETY: direct character comparison avoids temporary buffer allocation
        // and eliminates any potential string_view lifetime issues.
        for (const auto& op : TWO_CHAR_OPERATORS)
        {
            if (op.symbol.length() == 2 &&
                op.symbol[0] == firstChar &&
                op.symbol[1] == secondChar)
            {
                pos = tempPos + 1;
                return TokenFactory::createOperatorToken(op.type, op.symbol, location);
            }
        }

        return TokenFactory::createEndToken(location);
    }

    Token Lexer::parseInterpolatedString()
    {
        errors::SourceLocation location = locationTracker->getCurrentLocation();
        advance(); // skip '$'
        advance(); // skip '"'

        std::string text;
        while (pos < input.length() && input[pos] != '"' && input[pos] != '{')
        {
            if (input[pos] == '\\' && pos + 1 < input.length())
            {
                advance();
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
            // No interpolation found, treat as regular string. `text` is a
            // local std::string (escapes already processed); the token needs
            // a string_view that outlives this function, so move into arena.
            advance();
            return TokenFactory::createStringToken(storeProcessed(std::move(text)), location);
        }

        advance(); // skip '{'
        interpolationState.active = true;
        interpolationState.braceDepth = 0;
        auto sv = storeProcessed(std::move(text));
        return Token{TokenType::INTERP_STRING_BEGIN, 0.0, 0,
                     sv, static_cast<uint32_t>(sv.size()), location};
    }

    Token Lexer::scanInterpolatedSegment()
    {
        errors::SourceLocation location = locationTracker->getCurrentLocation();
        std::string text;

        while (pos < input.length() && input[pos] != '"' && input[pos] != '{')
        {
            if (input[pos] == '\\' && pos + 1 < input.length())
            {
                advance();
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
            advance();
            interpolationState.braceDepth = 0;
            auto svMid = storeProcessed(std::move(text));
            return Token{TokenType::INTERP_STRING_MIDDLE, 0.0, 0,
                         svMid, static_cast<uint32_t>(svMid.size()), location};
        }

        advance(); // skip closing '"'
        interpolationState.active = false;
        auto svEnd = storeProcessed(std::move(text));
        return Token{TokenType::INTERP_STRING_END, 0.0, 0,
                     svEnd, static_cast<uint32_t>(svEnd.size()), location};
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

    Token Lexer::peekAhead(size_t offset)
    {
        if (offset == 0)
        {
            return peekNextToken();
        }

        LexerState savedState = captureState();

        Token result;
        for (size_t i = 0; i < offset; ++i)
        {
            result = getNextToken();

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
