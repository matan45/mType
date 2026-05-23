#include "LspDiagnosticConverter.hpp"

#include "UriUtils.hpp"

#include <algorithm>

namespace mtype::lsp
{
    namespace
    {
        // Compiler positions are 1-based; LSP positions are 0-based.
        // Clamp to 0 in case the source ever passes garbage.
        int toLspLine(int oneBasedLine)
        {
            return std::max(0, oneBasedLine - 1);
        }

        int toLspColumn(int oneBasedColumn)
        {
            return std::max(0, oneBasedColumn - 1);
        }

        Range toLspRange(const errors::SourceLocation& start,
                         const errors::SourceLocation& end)
        {
            Range r;
            r.start.line = toLspLine(start.getLine());
            r.start.character = toLspColumn(start.getColumn());

            // For point spans (end == start), highlight a single character
            // so VS Code shows a visible squiggle.
            if (end.getLine() == start.getLine()
                && end.getColumn() <= start.getColumn())
            {
                r.end.line = r.start.line;
                r.end.character = r.start.character + 1;
            }
            else
            {
                r.end.line = toLspLine(end.getLine());
                r.end.character = toLspColumn(end.getColumn());
            }
            return r;
        }

        int toLspSeverity(diagnostics::Severity sev)
        {
            // Severity values were chosen to match LSP DiagnosticSeverity.
            return static_cast<int>(sev);
        }

        // Best-effort: if the source location's filename looks like a real
        // path that the LSP isn't tracking under documentUri, convert it
        // to a file:// URI; otherwise return the document URI we already
        // know. The LSP only complains about secondary spans whose URI
        // refers to an open document, so this is forgiving.
        std::string filenameToUri(const std::string& filename,
                                  const std::string& fallbackUri)
        {
            if (filename.empty() || filename == "<unknown>")
            {
                return fallbackUri;
            }
            // If the filename already looks like a URI, pass it through.
            if (filename.rfind("file://", 0) == 0
                || filename.rfind("untitled:", 0) == 0)
            {
                return filename;
            }
            return UriUtils::filePathToUri(filename);
        }
    } // namespace

    Diagnostic toLspDiagnostic(const diagnostics::Diagnostic& coreDiag,
                               const std::string& documentUri)
    {
        Diagnostic out;
        out.severity = toLspSeverity(coreDiag.severity);
        out.message = coreDiag.message;
        out.source = "mType";

        // Range from the primary span. If the diagnostic has no location,
        // we still emit a stable {0,0}-{0,1} range so VS Code displays it
        // somewhere — the renderer-equivalent for "no caret" output.
        if (coreDiag.hasLocation())
        {
            out.range = toLspRange(coreDiag.primarySpan.start,
                                    coreDiag.primarySpan.end);
        }
        else
        {
            out.range.start.line = 0;
            out.range.start.character = 0;
            out.range.end.line = 0;
            out.range.end.character = 1;
        }

        // Error code: publish the stable id (e.g., "MT-E1001"). VS Code
        // shows it next to the message and uses it for the link in the
        // diagnostic hover.
        if (coreDiag.code != nullptr)
        {
            out.code = std::string(coreDiag.code->id);
        }

        // LSP DiagnosticTag values are integers (1 = Unnecessary, 2 = Deprecated)
        // and the core severity-equivalent enum. Pass through verbatim.
        out.tags = coreDiag.tags;

        // Secondary spans → relatedInformation. Each carries a label and
        // a Location pointing back into the source file.
        for (const auto& secondary : coreDiag.secondarySpans)
        {
            DiagnosticRelatedInformation ri;
            ri.location.uri = filenameToUri(
                secondary.start.getFilename(), documentUri);
            ri.location.range = toLspRange(secondary.start, secondary.end);
            ri.message = secondary.label.empty()
                ? coreDiag.message
                : secondary.label;
            out.relatedInformation.push_back(std::move(ri));
        }

        // Surface notes as related information too — they don't have
        // their own location, so they reuse the primary span. This makes
        // them visible in VS Code's "Problems" panel as expandable items.
        for (const auto& note : coreDiag.notes)
        {
            DiagnosticRelatedInformation ri;
            ri.location.uri = documentUri;
            ri.location.range = out.range;
            ri.message = note;
            out.relatedInformation.push_back(std::move(ri));
        }

        // Data blob — round-trips the exception type and any future
        // payload through textDocument/codeAction so the action handler
        // can dispatch on it. Keys chosen to match a stable schema.
        json data = json::object();
        if (coreDiag.code != nullptr)
        {
            data["codeId"] = std::string(coreDiag.code->id);
        }
        if (!coreDiag.sourceExceptionType.empty())
        {
            data["exceptionType"] = coreDiag.sourceExceptionType;
        }
        if (!coreDiag.suggestions.empty())
        {
            json suggestionsArr = json::array();
            for (const auto& s : coreDiag.suggestions)
            {
                json sj = json::object();
                if (!s.label.empty()) sj["label"] = s.label;
                if (!s.renderedHint.empty()) sj["hint"] = s.renderedHint;
                if (!s.edits.empty())
                {
                    // Convert 1-based source coordinates to 0-based LSP
                    // coordinates directly. We deliberately do NOT use
                    // toLspRange here — its point-span widening
                    // (start == end → end = start+1) would corrupt
                    // zero-width inserts (e.g. the "insert ';'" edit
                    // emitted by convertMissingSemicolon). Zero-width
                    // ranges are intentional and must round-trip verbatim
                    // so the code-action handler can apply them as inserts
                    // rather than as 1-char overwrites of the next token
                    // (MYT-364).
                    json editsArr = json::array();
                    for (const auto& e : s.edits)
                    {
                        json ej = json::object();
                        ej["start"] = {
                            {"line", toLspLine(e.start.getLine())},
                            {"character", toLspColumn(e.start.getColumn())}
                        };
                        ej["end"] = {
                            {"line", toLspLine(e.end.getLine())},
                            {"character", toLspColumn(e.end.getColumn())}
                        };
                        ej["newText"] = e.newText;
                        editsArr.push_back(std::move(ej));
                    }
                    sj["edits"] = std::move(editsArr);
                }
                sj["applicability"] = static_cast<int>(s.applicability);
                suggestionsArr.push_back(std::move(sj));
            }
            data["suggestions"] = std::move(suggestionsArr);
        }
        if (!data.empty())
        {
            out.data = data;
        }

        return out;
    }
}
