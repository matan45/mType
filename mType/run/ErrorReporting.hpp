#pragma once
#include <exception>
#include "../diagnostics/DiagnosticRenderer.hpp"

namespace runMain
{
    /**
     * Set the global color mode used by `reportException`. Call once,
     * after parsing the --no-color / --color CLI flags. Defaults to Auto
     * (TTY-detect + NO_COLOR aware) if never set.
     */
    void setColorMode(diagnostics::DiagnosticRenderer::ColorMode mode);

    /**
     * Convert a thrown exception into a Diagnostic and render it to
     * std::cerr using the configured color mode. Each call constructs a
     * fresh renderer (cheap; this is a cold path).
     */
    void reportException(const std::exception& e);
}
