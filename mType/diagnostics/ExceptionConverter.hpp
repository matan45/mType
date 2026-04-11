#pragma once
#include <exception>
#include "Diagnostic.hpp"

namespace diagnostics
{
    /**
     * Convert any thrown exception into a Diagnostic.
     *
     * The function dispatches via dynamic_cast across every known
     * ScriptException subclass and produces a Diagnostic with the right
     * error code, severity, primary span, and optional secondary spans /
     * notes pulled from the exception's structured fields.
     *
     * Fall-back behavior:
     *   - ScriptException without a more specific match → InternalError
     *     code MT-E9000 with the exception's existing message.
     *   - std::exception not derived from ScriptException → MT-E9000 with
     *     a synthetic "<unknown>" location.
     *
     * This is the single place that knows about every exception type. Any
     * new exception subclass should add a `case` here so the renderer can
     * give it a stable code and structured spans.
     */
    Diagnostic fromException(const std::exception& e);
}
