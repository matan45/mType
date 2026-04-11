#pragma once
#include <cstdio>
#include <ostream>
#include <vector>
#include "Diagnostic.hpp"

namespace diagnostics
{
    /**
     * Rust-style terminal renderer for Diagnostic.
     *
     * Output layout (matches rustc visually):
     *
     *   error[MT-E1001]: cannot find value 'foa' in this scope
     *     --> src/main.mt:14:7
     *      |
     *   14 |     foa.doThing();
     *      |     ^^^ not found in this scope
     *      |
     *      = note: a related fact about the error
     *      = help: did you mean 'foo'?
     *
     * Source lines come from SourceFileCache (populated by the lexer at
     * parse time and by the LSP DocumentManager for in-memory buffers).
     * Diagnostics whose primary span has no resolvable file fall back to
     * a header-only render with no snippet block.
     *
     * Color decisions (auto/always/never) are made by the caller via the
     * Options struct; the renderer itself only emits escapes when
     * `colorEnabled == true`.
     */
    class DiagnosticRenderer
    {
    public:
        enum class ColorMode
        {
            Auto,    // emit color iff stream is a TTY and NO_COLOR is unset
            Always,  // always emit color
            Never    // never emit color
        };

        struct Options
        {
            std::ostream* stream = nullptr;     // defaults to std::cerr
            std::FILE* ttyHandle = nullptr;     // matched against `stream` for TTY detection
            ColorMode colorMode = ColorMode::Auto;
            int contextLinesBefore = 0;         // extra source lines above the caret
            int contextLinesAfter = 0;
        };

        /**
         * Build an Options struct from the environment:
         *   - `stream` defaults to std::cerr, `ttyHandle` to stderr
         *   - color is Auto (TTY + NO_COLOR aware)
         *   - context lines stay 0 (matches rustc default)
         *
         * Callers can override individual fields after construction.
         */
        static Options defaultOptions();

        explicit DiagnosticRenderer(Options options);

        /// Render a single diagnostic.
        void render(const Diagnostic& diagnostic);

        /// Render multiple diagnostics in order.
        void renderAll(const std::vector<Diagnostic>& diagnostics);

        /// True if this renderer will emit ANSI color escapes.
        bool colorEnabled() const { return colorEnabled_; }

    private:
        void renderHeader(const Diagnostic& diagnostic);
        void renderArrow(const LabeledSpan& span, int gutterWidth);
        void renderSnippet(const LabeledSpan& span,
                           int gutterWidth,
                           bool isPrimary);
        void renderNotes(const Diagnostic& diagnostic, int gutterWidth);
        void renderStackTrace(const Diagnostic& diagnostic, int gutterWidth);

        // Helpers
        int gutterWidthFor(const Diagnostic& diagnostic) const;
        std::string severityLabel(Severity severity) const;
        const char* severityColor(Severity severity) const;
        std::string colorize(std::string_view text, std::string_view color) const;
        std::string bold(std::string_view text) const;
        std::string dim(std::string_view text) const;

        // Resolve the platform color decision (TTY + NO_COLOR + ColorMode).
        // Also enables Windows VT processing on success so escape sequences
        // are interpreted by the console.
        static bool decideColor(const Options& opts);

        Options options_;
        std::ostream* out_;
        bool colorEnabled_;
    };
}
