#pragma once
#include <cstdint>
#include <string_view>
#include "TokenType.hpp"
#include "../errors/SourceLocation.hpp"

namespace token
{
    // Token::stringValue is a non-owning view.
    //
    // For identifier/keyword/operator/numeric-lexeme tokens it points into the
    // owning Lexer's `input` source buffer.
    //
    // For STRING_LITERAL tokens whose text needs post-processing (escape
    // sequences) and for the three interpolated-string segment tokens
    // (INTERP_STRING_BEGIN/MIDDLE/END), it points into the Lexer's
    // `processedLiteralArena` (std::deque<std::string> with stable addresses).
    //
    // In both cases the view is valid only while the owning Lexer is alive.
    // Consumers that need to retain the lexeme past the Lexer's lifetime
    // (e.g., AST nodes) must copy into an owned std::string before the Lexer
    // goes out of scope. See MYT-130.
    //
    // `length` mirrors `stringValue.size()` at construction and remains valid
    // after the Lexer's source buffer is destroyed. Prefer it over
    // `stringValue.size()` whenever the lexer is gone — `string_view::size()`
    // on a view with a dangling data pointer is technically UB on the
    // implementation level even though every mainstream stdlib stores the
    // size as a plain integer field.
    struct Token
    {
        TokenType type;
        double floatValue;
        int64_t intValue;
        std::string_view stringValue;
        uint32_t length;
        errors::SourceLocation location;
    };
}
