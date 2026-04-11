#pragma once
#include <string_view>
#include "Severity.hpp"

namespace diagnostics
{
    /**
     * A stable identifier for a diagnostic kind.
     *
     * Codes are defined once in ErrorCodeRegistry.hpp via an X-macro table
     * and exposed as `constexpr`-friendly globals under
     * `diagnostics::codes::*`. The Diagnostic struct stores a pointer to one
     * of these globals; equality and hashing on the pointer is sufficient.
     *
     * Numbering policy: codes are append-only. Once an `id` is published,
     * its numeric value must never be reused for a different meaning.
     * Deprecated codes become reserved entries that keep their `id`.
     */
    struct ErrorCode
    {
        std::string_view id;              // e.g. "MT-E1001"
        Severity defaultSeverity;
        std::string_view title;           // short headline used by the renderer
    };
}
