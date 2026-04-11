#include "ErrorReporting.hpp"

#include "../diagnostics/ExceptionConverter.hpp"

namespace runMain
{
    namespace
    {
        diagnostics::DiagnosticRenderer::ColorMode g_colorMode =
            diagnostics::DiagnosticRenderer::ColorMode::Auto;
    }

    void setColorMode(diagnostics::DiagnosticRenderer::ColorMode mode)
    {
        g_colorMode = mode;
    }

    void reportException(const std::exception& e)
    {
        auto options = diagnostics::DiagnosticRenderer::defaultOptions();
        options.colorMode = g_colorMode;
        diagnostics::DiagnosticRenderer renderer(options);
        renderer.render(diagnostics::fromException(e));
    }

    void reportWarning(const diagnostics::Diagnostic& warning)
    {
        auto options = diagnostics::DiagnosticRenderer::defaultOptions();
        options.colorMode = g_colorMode;
        diagnostics::DiagnosticRenderer renderer(options);
        renderer.render(warning);
    }

    void reportWarnings(const std::vector<diagnostics::Diagnostic>& warnings)
    {
        if (warnings.empty()) return;
        auto options = diagnostics::DiagnosticRenderer::defaultOptions();
        options.colorMode = g_colorMode;
        diagnostics::DiagnosticRenderer renderer(options);
        for (const auto& w : warnings) {
            renderer.render(w);
        }
    }
}
