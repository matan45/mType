#include "TokenStream.hpp"
#include "../errors/ParseException.hpp"
#include "../lexer/TokenFactory.hpp"

namespace parser
{
    using namespace errors;

    TokenStream::TokenStream(Lexer& lex) : lexer(lex)
    {
        // Initialize with first token
        try
        {
            advance();
        }
        catch (const errors::ParseException&)
        {
            // Set END token and re-throw to prevent parser from starting
            currentToken = TokenFactory::createEndToken(SourceLocation("", 0, 0));
            throw;
        }
        catch (const std::exception&)
        {
            // Set END token and re-throw to prevent parser from starting
            currentToken = TokenFactory::createEndToken(SourceLocation("", 0, 0));
            throw;
        }
    }

    void TokenStream::advance()
    {
        if (lookaheadCount > 0)
        {
            currentToken = lookahead[lookaheadHead];
            lookaheadHead = (lookaheadHead + 1) % LOOKAHEAD_CAPACITY;
            --lookaheadCount;
            return;
        }

        try
        {
            currentToken = lexer.getNextToken();
        }
        catch (const ParseException&)
        {
            throw;
        }
        catch (const std::exception&)
        {
            // Create an END token to prevent infinite loops in case exception isn't caught
            currentToken = TokenFactory::createEndToken(SourceLocation("", 0, 0));
            throw;
        }
    }

    bool TokenStream::check(TokenType type) const noexcept
    {
        return currentToken.type == type;
    }

    bool TokenStream::isAtEnd() const noexcept
    {
        return currentToken.type == TokenType::END;
    }

    bool TokenStream::match(TokenType type)
    {
        if (check(type))
        {
            advance();
            return true;
        }
        return false;
    }

    void TokenStream::expect(TokenType type)
    {
        if (!match(type))
        {
            throw ParseException("Expected token type but found different type",
                                 currentToken.location);
        }
    }

    void TokenStream::expectGreaterForGeneric()
    {
        // If we have a pending > from a split >>, consume it
        if (hasRightShiftPending)
        {
            hasRightShiftPending = false;
            // The current token is already the next real token (advanced after splitting)
            // Just return - we've "consumed" the virtual > that was left over
            return;
        }

        if (check(TokenType::GREATER))
        {
            advance();
            return;
        }

        // Handle >> as two > tokens (e.g., Stream<List<T>>)
        if (check(TokenType::RIGHT_SHIFT))
        {
            // Mark that we have a pending > from splitting >>
            hasRightShiftPending = true;
            // Advance to the next real token
            advance();
            return;
        }

        throw ParseException("Expected '>' for generic type but found different token",
                             currentToken.location);
    }

    bool TokenStream::checkGreaterForGeneric() const noexcept
    {
        // If we have a pending > from splitting >>, we can match a >
        if (hasRightShiftPending)
        {
            return true;
        }
        return check(TokenType::GREATER) || check(TokenType::RIGHT_SHIFT);
    }

    Token TokenStream::peek() const
    {
        return peekAhead(0);
    }

    Token TokenStream::peekAhead(size_t offset) const
    {
        // Legacy Lexer::peekAhead semantics (preserved for all callers):
        //   peekAhead(0) == peekAhead(1) == first token after currentToken
        //   peekAhead(N) for N >= 2 == Nth token after currentToken
        // Normalize to a 0-based ring index.
        const size_t ringIndex = (offset == 0) ? 0 : (offset - 1);

        // Rare path: deeper than the ring. Ask the lexer directly; it
        // captures/restores its own state. The lexer's cursor is already past
        // the tokens we have buffered.
        if (ringIndex >= LOOKAHEAD_CAPACITY)
        {
            return lexer.peekAhead(ringIndex - lookaheadCount + 1);
        }

        // Refill from the lexer until the requested slot is buffered.
        while (lookaheadCount <= ringIndex)
        {
            const size_t tail = (lookaheadHead + lookaheadCount) % LOOKAHEAD_CAPACITY;
            lookahead[tail] = lexer.getNextToken();
            ++lookaheadCount;

            if (lookahead[tail].type == TokenType::END)
            {
                // Pad remaining requested slots with END so peekAhead past EOF
                // keeps returning END without re-driving the lexer.
                while (lookaheadCount <= ringIndex && lookaheadCount < LOOKAHEAD_CAPACITY)
                {
                    const size_t pad = (lookaheadHead + lookaheadCount) % LOOKAHEAD_CAPACITY;
                    lookahead[pad] = lookahead[tail];
                    ++lookaheadCount;
                }
                break;
            }
        }

        return lookahead[(lookaheadHead + ringIndex) % LOOKAHEAD_CAPACITY];
    }
}
