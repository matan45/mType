#pragma once

#include <string>
#include "LSPTypes.hpp"
#include "../../../mType/diagnostics/Diagnostic.hpp"

namespace mtype::lsp
{
    /**
     * Converts the shared `diagnostics::Diagnostic` data model into the
     * LSP wire type `lsp::Diagnostic`. This is the only place that
     * translates between the two — the rest of the LSP server should
     * never look at exception strings to derive locations.
     *
     * The translator:
     *   - Maps line/column 1-based (compiler) to 0-based (LSP)
     *   - Sets `range.end` from the diagnostic's primary span end (or
     *     start+1 for point spans)
     *   - Carries `code` (the MT-Exxxx id), `relatedInformation` (one
     *     entry per secondary span, plus one per note for visibility),
     *     and a `data` blob with the source exception type so the
     *     CodeActionHandler can route quick fixes
     *   - Falls back gracefully when the diagnostic has no resolvable
     *     file (the renderer's `hasLocation()` invariant)
     *
     * @param coreDiag    Source diagnostic from `diagnostics::fromException`
     *                    or any analyzer that produces the shared model.
     * @param documentUri The LSP document URI for the file the diagnostic
     *                    refers to. Used as the base when no other file
     *                    is specified, and as the URI in
     *                    `relatedInformation[].location.uri`.
     */
    Diagnostic toLspDiagnostic(const diagnostics::Diagnostic& coreDiag,
                               const std::string& documentUri);
}
