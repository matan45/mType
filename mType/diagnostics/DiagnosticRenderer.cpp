#include "DiagnosticRenderer.hpp"
#include <cstddef>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#include "AnsiStyle.hpp"
#include "ErrorCode.hpp"
#include "SourceFileCache.hpp"
#include "TerminalDetect.hpp"

namespace diagnostics
{
    // ----- Options ----------------------------------------------------------

    DiagnosticRenderer::Options DiagnosticRenderer::defaultOptions()
    {
        Options o;
        o.stream = &std::cerr;
        o.ttyHandle = stderr;
        o.colorMode = ColorMode::Auto;
        return o;
    }

    bool DiagnosticRenderer::decideColor(const Options& opts)
    {
        switch (opts.colorMode)
        {
        case ColorMode::Never:
            return false;
        case ColorMode::Always:
            // Even when "always", attempt to enable VT on Windows so the
            // emitted escapes are interpreted rather than printed literally.
            TerminalDetect::enableVirtualTerminalProcessing(opts.ttyHandle);
            return true;
        case ColorMode::Auto:
        default:
            if (TerminalDetect::noColorRequested())
            {
                return false;
            }
            if (!TerminalDetect::isTerminal(opts.ttyHandle))
            {
                return false;
            }
            TerminalDetect::enableVirtualTerminalProcessing(opts.ttyHandle);
            return true;
        }
    }

    // ----- ctor / lifetime --------------------------------------------------

    DiagnosticRenderer::DiagnosticRenderer(Options options)
        : options_(options),
          out_(options.stream ? options.stream : &std::cerr),
          colorEnabled_(decideColor(options))
    {
    }

    // ----- color helpers ----------------------------------------------------

    std::string DiagnosticRenderer::colorize(std::string_view text,
                                              std::string_view color) const
    {
        if (!colorEnabled_)
        {
            return std::string(text);
        }
        std::string result;
        result.reserve(text.size() + color.size() + ansi::RESET.size());
        result.append(color);
        result.append(text);
        result.append(ansi::RESET);
        return result;
    }

    std::string DiagnosticRenderer::bold(std::string_view text) const
    {
        return colorEnabled_
            ? std::string(ansi::BOLD).append(text).append(ansi::RESET)
            : std::string(text);
    }

    std::string DiagnosticRenderer::dim(std::string_view text) const
    {
        return colorEnabled_
            ? std::string(ansi::DIM).append(text).append(ansi::RESET)
            : std::string(text);
    }

    const char* DiagnosticRenderer::severityColor(Severity severity) const
    {
        switch (severity)
        {
        case Severity::Error:   return ansi::BRIGHT_RED.data();
        case Severity::Warning: return ansi::BRIGHT_YELLOW.data();
        case Severity::Note:    return ansi::BRIGHT_BLUE.data();
        case Severity::Help:    return ansi::BRIGHT_CYAN.data();
        }
        return ansi::BRIGHT_RED.data();
    }

    std::string DiagnosticRenderer::severityLabel(Severity severity) const
    {
        switch (severity)
        {
        case Severity::Error:   return "error";
        case Severity::Warning: return "warning";
        case Severity::Note:    return "note";
        case Severity::Help:    return "help";
        }
        return "error";
    }

    // ----- top-level dispatch -----------------------------------------------

    void DiagnosticRenderer::render(const Diagnostic& diagnostic)
    {
        const int gutterWidth = gutterWidthFor(diagnostic);

        renderHeader(diagnostic);

        if (diagnostic.hasLocation())
        {
            renderArrow(diagnostic.primarySpan, gutterWidth);
            renderSnippet(diagnostic.primarySpan, gutterWidth, true);

            for (const auto& secondary : diagnostic.secondarySpans)
            {
                if (!secondary.start.getFilename().empty()
                    && secondary.start.getFilename() != "<unknown>")
                {
                    renderArrow(secondary, gutterWidth);
                    renderSnippet(secondary, gutterWidth, false);
                }
            }
        }

        renderNotes(diagnostic, gutterWidth);
        renderStackTrace(diagnostic, gutterWidth);
        (*out_) << "\n";
        out_->flush();
    }

    void DiagnosticRenderer::renderAll(const std::vector<Diagnostic>& diagnostics)
    {
        for (const auto& d : diagnostics)
        {
            render(d);
        }
    }

    // ----- gutter -----------------------------------------------------------

    int DiagnosticRenderer::gutterWidthFor(const Diagnostic& d) const
    {
        int maxLine = d.primarySpan.start.getLine();
        if (d.primarySpan.end.getLine() > maxLine)
        {
            maxLine = d.primarySpan.end.getLine();
        }
        for (const auto& s : d.secondarySpans)
        {
            if (s.start.getLine() > maxLine) maxLine = s.start.getLine();
            if (s.end.getLine() > maxLine) maxLine = s.end.getLine();
        }
        // Width of the line number itself, with a sane minimum so the
        // " --> file:line:col" arrow lines up consistently.
        int width = 1;
        for (int n = std::max(1, maxLine); n >= 10; n /= 10)
        {
            ++width;
        }
        return std::max(width, 2);
    }

    // ----- header -----------------------------------------------------------

    void DiagnosticRenderer::renderHeader(const Diagnostic& d)
    {
        // "error[MT-E1001]: cannot find value 'foa' in this scope"
        const std::string label = severityLabel(d.severity);
        std::string codeChunk;
        if (d.code != nullptr)
        {
            codeChunk = "[" + std::string(d.code->id) + "]";
        }
        const std::string heading = label + codeChunk;
        const std::string colored = colorize(heading, severityColor(d.severity));

        // Bold the message body to match rustc.
        const std::string boldHeading = colorEnabled_
            ? std::string(ansi::BOLD).append(colored).append(ansi::RESET)
            : colored;

        (*out_) << boldHeading << ": " << bold(d.message) << "\n";
    }

    // ----- arrow line -------------------------------------------------------

    void DiagnosticRenderer::renderArrow(const LabeledSpan& span, int gutterWidth)
    {
        // "  --> file:line:col"
        const std::string padding(gutterWidth, ' ');
        std::ostringstream loc;
        loc << span.start.getFilename()
            << ":" << span.start.getLine()
            << ":" << span.start.getColumn();
        (*out_) << padding << colorize("--> ", ansi::BRIGHT_BLUE)
                << loc.str() << "\n";
    }

    // ----- snippet ----------------------------------------------------------

    void DiagnosticRenderer::renderSnippet(const LabeledSpan& span,
                                            int gutterWidth,
                                            bool isPrimary)
    {
        const auto& cache = SourceFileCache::instance();
        const std::string& filename = span.start.getFilename();
        const int line = span.start.getLine();
        const int col = std::max(1, span.start.getColumn());

        auto sourceLine = cache.getLine(filename, line);

        const std::string padding(gutterWidth, ' ');
        const std::string blankGutter = padding + " " + colorize("|", ansi::BRIGHT_BLUE);

        // Empty gutter line above the snippet for breathing room.
        (*out_) << blankGutter << "\n";

        // The numbered line itself. If we couldn't find the source line in
        // the cache, render an em-dash placeholder so the layout still
        // shows the file:line and the caret label.
        std::ostringstream lineNumber;
        lineNumber.width(gutterWidth);
        lineNumber << line;
        (*out_) << colorize(lineNumber.str(), ansi::BRIGHT_BLUE)
                << " " << colorize("|", ansi::BRIGHT_BLUE) << " "
                << (sourceLine ? *sourceLine : std::string("<source unavailable>"))
                << "\n";

        // Caret line. Width = max(1, span columns on the same line).
        int caretWidth = 1;
        if (span.end.getLine() == span.start.getLine()
            && span.end.getColumn() > span.start.getColumn())
        {
            caretWidth = span.end.getColumn() - span.start.getColumn();
        }
        const char caretGlyph = isPrimary ? '^' : '-';
        std::string caret(static_cast<size_t>(caretWidth), caretGlyph);
        std::string spaces(static_cast<size_t>(col - 1), ' ');

        const char* caretColor = isPrimary
            ? severityColor(Severity::Error)
            : ansi::BRIGHT_BLUE.data();

        (*out_) << padding << " " << colorize("|", ansi::BRIGHT_BLUE) << " "
                << spaces << colorize(caret, caretColor);

        if (!span.label.empty())
        {
            (*out_) << " " << colorize(span.label, caretColor);
        }
        (*out_) << "\n";
    }

    // ----- notes / hints ----------------------------------------------------

    void DiagnosticRenderer::renderNotes(const Diagnostic& d, int gutterWidth)
    {
        if (d.notes.empty() && d.suggestions.empty())
        {
            return;
        }

        const std::string padding(gutterWidth, ' ');

        for (const auto& note : d.notes)
        {
            (*out_) << padding << " "
                    << colorize("=", ansi::BRIGHT_BLUE) << " "
                    << bold("note") << ": " << note << "\n";
        }

        for (const auto& suggestion : d.suggestions)
        {
            (*out_) << padding << " "
                    << colorize("=", ansi::BRIGHT_BLUE) << " "
                    << bold("help") << ": "
                    << (suggestion.renderedHint.empty()
                        ? suggestion.label
                        : suggestion.renderedHint)
                    << "\n";
        }
    }

    // ----- stack trace ------------------------------------------------------

    void DiagnosticRenderer::renderStackTrace(const Diagnostic& d, int gutterWidth)
    {
        if (d.stackTrace.empty())
        {
            return;
        }
        const std::string padding(gutterWidth, ' ');
        (*out_) << padding << " "
                << colorize("=", ansi::BRIGHT_BLUE) << " "
                << bold("stack trace") << ":\n";
        for (const auto& frame : d.stackTrace)
        {
            (*out_) << padding << "     " << dim(frame) << "\n";
        }
    }
}
