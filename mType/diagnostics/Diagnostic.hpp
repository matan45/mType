#pragma once
#include <string>
#include <vector>
#include "../errors/SourceLocation.hpp"
#include "ErrorCode.hpp"
#include "Severity.hpp"

namespace diagnostics
{
    /**
     * How safe a quick fix is to apply automatically.
     * The LSP code action handler decides whether to apply silently, ask
     * the user, or merely surface the suggestion as text.
     */
    enum class FixApplicability
    {
        MachineApplicable,  // safe to apply without user intervention
        HasPlaceholders,    // contains TODOs the user must fill in
        MaybeIncorrect      // heuristic — show as suggestion only
    };

    /**
     * A textual edit applied to a single source range.
     * Both endpoints reference the same file (LabeledSpan handles cross-file
     * edits via separate Suggestion entries).
     */
    struct TextEdit
    {
        errors::SourceLocation start;
        errors::SourceLocation end;
        std::string newText;
    };

    /**
     * A suggested fix attached to a Diagnostic. Carries both a human
     * representation (renderedHint, used by the CLI renderer) and a
     * machine representation (edits, used by the LSP code action handler).
     * Either may be empty depending on what the converter knows.
     */
    struct Suggestion
    {
        std::string label;                  // short title (e.g. "did you mean 'foo'?")
        std::string renderedHint;           // pre-formatted hint line for the CLI
        std::vector<TextEdit> edits;        // optional structured edits
        FixApplicability applicability = FixApplicability::MaybeIncorrect;
    };

    /**
     * A point or range in source with an optional label printed under the
     * caret. The primary span carries the main caret; secondary spans show
     * supporting locations (e.g. "first declared here").
     *
     * For point locations, leave `end == start` (the renderer will draw a
     * single-character caret). Multi-line spans set `end` to the line/column
     * after the last character.
     */
    struct LabeledSpan
    {
        errors::SourceLocation start;
        errors::SourceLocation end;
        std::string label;
        bool isPrimary = false;

        LabeledSpan() = default;

        explicit LabeledSpan(const errors::SourceLocation& point)
            : start(point), end(point)
        {
        }

        LabeledSpan(const errors::SourceLocation& s, const errors::SourceLocation& e)
            : start(s), end(e)
        {
        }
    };

    /**
     * The shared diagnostic data model.
     *
     * Produced by ExceptionConverter (catching ScriptException subclasses)
     * or by analyzers that want to emit warnings directly. Consumed by
     * DiagnosticRenderer (CLI) and LspDiagnosticConverter (LSP).
     *
     * Both surfaces share this single representation so they can never
     * drift apart.
     */
    struct Diagnostic
    {
        // Identity
        const ErrorCode* code = nullptr;
        Severity severity = Severity::Error;

        // Headline
        std::string message;

        // Spans
        LabeledSpan primarySpan;
        std::vector<LabeledSpan> secondarySpans;

        // Supplementary text
        std::vector<std::string> notes;       // "= note: ..." lines under the snippet
        std::vector<Suggestion> suggestions;  // "= help: ..." lines + LSP quick fixes

        // Runtime call stack (UserException only — empty otherwise)
        std::vector<std::string> stackTrace;

        // Diagnostic aid: the C++ type that produced this diagnostic, for the
        // LSP `data` blob and for tests. Empty when not constructed from an
        // exception.
        std::string sourceExceptionType;

        bool hasLocation() const
        {
            return !primarySpan.start.getFilename().empty()
                && primarySpan.start.getFilename() != "<unknown>";
        }
    };
}
