#pragma once

namespace diagnostics
{
    /**
     * Severity of a diagnostic message.
     * Mirrors LSP DiagnosticSeverity values (Error=1..Hint=4) so the LSP
     * converter can map directly without remapping.
     */
    enum class Severity
    {
        Error = 1,
        Warning = 2,
        Note = 3,
        Help = 4
    };
}
