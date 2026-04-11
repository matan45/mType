#pragma once
#include <exception>
#include <vector>
#include "../diagnostics/Diagnostic.hpp"
#include "../diagnostics/DiagnosticRenderer.hpp"

namespace runMain
{
    /**
     * Set the global color mode used by `reportException` and
     * `reportWarning`. Call once after parsing the --no-color / --color
     * CLI flags. Defaults to Auto (TTY-detect + NO_COLOR aware).
     */
    void setColorMode(diagnostics::DiagnosticRenderer::ColorMode mode);

    /**
     * Convert a thrown exception into a Diagnostic and render it to
     * std::cerr using the configured color mode. Each call constructs a
     * fresh renderer (cheap; this is a cold path).
     */
    void reportException(const std::exception& e);

    /**
     * Render a single non-fatal diagnostic (warning, note, hint) to
     * std::cerr using the configured color mode. Does not exit. Used by
     * the CLI driver to drain BytecodeCompiler::warnings() after a
     * successful compile.
     */
    void reportWarning(const diagnostics::Diagnostic& warning);

    /**
     * Convenience: render every diagnostic in `warnings` in order.
     * Equivalent to calling `reportWarning` in a loop, but constructs
     * the renderer just once.
     */
    void reportWarnings(const std::vector<diagnostics::Diagnostic>& warnings);
}
