#pragma once

#include <string>
#include <vector>

#include "LSPTypes.hpp"
#include "../../../mType/errors/SourceLocation.hpp"
#include "../../../mType/token/Token.hpp"

namespace mtype::lsp::utils
{
    // Declaration ranges recovered from a token list. The AST records only
    // where a declaration starts, so the name and the full extent come from
    // the tokens of the same content. Shared by the document outline
    // (DocumentSymbolHandler) and the workspace type graph (MYT-432) so both
    // report identical ranges. All ranges are 0-based LSP ranges.

    // `selectionRange` for a named declaration: the first IDENTIFIER token
    // equal to `name` at or after `loc`. `name == "constructor"` matches the
    // CONSTRUCTOR keyword instead. Falls back to a zero-width range at `loc`.
    Range nameRange(const std::string& content,
                    const std::vector<token::Token>& toks,
                    const errors::SourceLocation& loc,
                    const std::string& name);

    // `range` for a declaration: from `loc` to the `;` that ends a
    // signature-only declaration, or to the `}` that closes its body.
    // Keeps `fallback.end` when neither is found, and ends at the last token
    // when the braces never balance (source truncated mid-edit).
    Range declRange(const std::string& content,
                    const std::vector<token::Token>& toks,
                    const errors::SourceLocation& loc,
                    const Range& fallback);

    // From the start of `nameR` to the end of its line.
    Range singleLineFallback(const std::string& content, const Range& nameR);
}
