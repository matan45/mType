#pragma once

#include <cstddef>
#include <string>

#include "../DocumentManager.hpp"
#include "../../../mType/token/Token.hpp"

namespace mtype::lsp::utils
{
    // Token::stringValue is a std::string_view into the lexer's source
    // buffer. DocumentManager::parseDocument creates a temporary lexer
    // just for the token list and lets it go out of scope before the
    // function returns — by the time any LSP handler runs, every
    // token's .data() pointer is dangling. Token::length stays valid
    // (it's just an integer field), so we can re-extract the lexeme
    // from doc->content using (line, column, length). This helper
    // does the arithmetic safely.
    //
    // Pulled out of two identical anonymous-namespace copies in
    // RenameHandler.cpp + InlayHintHandler.cpp so a future fix
    // (e.g., a multi-byte bug) only has to be applied once.
    inline std::string tokenText(const Document* doc, const token::Token& tok)
    {
        if (!doc) return {};
        int line = tok.location.getLine() - 1;
        int col = tok.location.getColumn() - 1;
        std::size_t len = tok.length;
        if (line < 0 || col < 0 || len == 0) return {};

        const std::string& content = doc->content;
        std::size_t pos = 0;
        int currentLine = 0;
        while (currentLine < line && pos < content.size())
        {
            if (content[pos] == '\n') currentLine++;
            pos++;
        }
        if (currentLine != line) return {};
        std::size_t start = pos + static_cast<std::size_t>(col);
        if (start + len > content.size()) return {};
        return content.substr(start, len);
    }
}
