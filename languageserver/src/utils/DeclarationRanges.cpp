#include "DeclarationRanges.hpp"

#include <cstddef>

#include "LSPUtils.hpp"
#include "../../../mType/token/TokenType.hpp"

namespace mtype::lsp::utils {

namespace {

using TT = token::TokenType;

// LSP positions are 0-based; SourceLocation is 1-based. Centralize so
// drift between handlers never bites us.
Position fromLoc(const errors::SourceLocation& loc) {
    Position p;
    p.line = loc.getLine() - 1;
    p.character = loc.getColumn() - 1;
    if (p.line < 0) p.line = 0;
    if (p.character < 0) p.character = 0;
    return p;
}

// Compute (line, column) offset (0-based) into `content`. Returns
// std::string::npos when the position is out of range.
std::size_t offsetOf(const std::string& content, int line, int character) {
    std::size_t pos = 0;
    int currentLine = 0;
    while (currentLine < line && pos < content.size()) {
        if (content[pos] == '\n') currentLine++;
        pos++;
    }
    if (currentLine != line) return std::string::npos;
    std::size_t off = pos + static_cast<std::size_t>(character);
    if (off > content.size()) return std::string::npos;
    return off;
}

// Returns the (line, character) 0-based position for an absolute byte
// offset into `content`.
Position positionFromOffset(const std::string& content, std::size_t offset) {
    Position p;
    p.line = 0;
    p.character = 0;
    if (offset > content.size()) offset = content.size();
    std::size_t lineStart = 0;
    for (std::size_t i = 0; i < offset; ++i) {
        if (content[i] == '\n') {
            p.line++;
            lineStart = i + 1;
        }
    }
    p.character = static_cast<int>(offset - lineStart);
    return p;
}

// Locate the first token at-or-after the given source location. Linear
// scan — documents are small and this only runs per declaration.
std::size_t findTokenIndexAt(const std::vector<token::Token>& toks,
                             const errors::SourceLocation& loc) {
    int line = loc.getLine();
    int col = loc.getColumn();
    for (std::size_t i = 0; i < toks.size(); ++i) {
        int tl = toks[i].location.getLine();
        int tc = toks[i].location.getColumn();
        if (tl > line || (tl == line && tc >= col)) {
            return i;
        }
    }
    return toks.size();
}

} // namespace

// The lexer drops comments entirely and emits string literals as
// STRING_LITERAL (not IDENTIFIER), so neither can produce a false
// positive — a substring scan of the raw source could. Falls back to a
// zero-width range at the start position if the token can't be located
// (e.g., stale AST after a destructive edit).
Range nameRange(const std::string& content,
                const std::vector<token::Token>& toks,
                const errors::SourceLocation& loc,
                const std::string& name) {
    Range r;
    r.start = fromLoc(loc);
    r.end = r.start;
    if (name.empty() || toks.empty()) return r;

    const bool wantConstructor = (name == "constructor");
    std::size_t idx = findTokenIndexAt(toks, loc);
    for (std::size_t i = idx; i < toks.size(); ++i) {
        const auto& tok = toks[i];
        if (wantConstructor) {
            if (tok.type != TT::CONSTRUCTOR) continue;
        } else {
            if (tok.type != TT::IDENTIFIER) continue;
            if (tok.length != name.size()) continue;
            if (tokenText(content, tok) != name) continue;
        }
        r.start.line = tok.location.getLine() - 1;
        r.start.character = tok.location.getColumn() - 1;
        r.end.line = r.start.line;
        r.end.character = r.start.character + static_cast<int>(tok.length);
        return r;
    }
    return r;
}

// Walks the tokens forward from the node's start location, counting brace
// depth across the body. Lexer emits strings/comments as whole tokens, so
// this is safe against `}` characters inside literals. Handles:
//   - block bodies (class/interface/method/ctor/function): end at the
//     matching RBRACE that closes the body.
//   - abstract methods / interface signatures with no body: end at the
//     SEMICOLON that terminates the signature.
//   - field-style decls with neither LBRACE nor SEMICOLON: degenerate
//     to the caller's fallback range.
Range declRange(const std::string& content,
                const std::vector<token::Token>& toks,
                const errors::SourceLocation& loc,
                const Range& fallback) {
    (void)content;
    Range r;
    r.start = fromLoc(loc);
    r.end = fallback.end;

    if (toks.empty()) return r;

    std::size_t idx = findTokenIndexAt(toks, loc);
    if (idx >= toks.size()) return r;

    // First pass: find either an LBRACE (block body) or a SEMICOLON
    // (signature-only declaration), whichever comes first.
    std::size_t scan = idx;
    while (scan < toks.size()) {
        TT t = toks[scan].type;
        if (t == TT::LBRACE) break;
        if (t == TT::SEMICOLON) {
            // Single-statement decl — end at the semicolon (inclusive).
            const auto& tok = toks[scan];
            Position end = fromLoc(tok.location);
            end.character += 1;  // SEMICOLON lexeme is one char
            r.end = end;
            return r;
        }
        scan++;
    }
    if (scan >= toks.size()) {
        // No body, no semicolon — leave fallback end in place. Caller
        // typically passes a same-line fallback so the range stays
        // visually localized.
        return r;
    }

    // Brace-depth walk from the first LBRACE to its matching RBRACE.
    int depth = 0;
    for (std::size_t k = scan; k < toks.size(); ++k) {
        TT t = toks[k].type;
        if (t == TT::LBRACE) depth++;
        else if (t == TT::RBRACE) {
            depth--;
            if (depth == 0) {
                const auto& tok = toks[k];
                Position end = fromLoc(tok.location);
                end.character += 1;  // RBRACE lexeme is one char
                r.end = end;
                return r;
            }
        }
    }

    // Unbalanced — source truncated mid-class. Fall back to the last
    // token's end position. Better than collapsing to selectionRange.
    const auto& last = toks.back();
    Position end = fromLoc(last.location);
    end.character += static_cast<int>(last.length);
    r.end = end;
    return r;
}

// Useful for fields and other declarations where declRange might not find
// a brace or semicolon. Spans from the name position to the end of the line.
Range singleLineFallback(const std::string& content, const Range& nameR) {
    Range r;
    r.start = nameR.start;
    r.end = nameR.end;

    std::size_t off = offsetOf(content, nameR.start.line, nameR.start.character);
    if (off == std::string::npos) return r;
    while (off < content.size() && content[off] != '\n') off++;
    r.end = positionFromOffset(content, off);
    return r;
}

} // namespace mtype::lsp::utils
