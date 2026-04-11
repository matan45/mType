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
}
