#pragma once

namespace tests::testFramework
{
    enum class TestType {
        NORMAL,
        ERROR_EXPECTED,
        OUTPUT_EXPECTED,
        PERFORMANCE,
        COMPILATION_TEST,
        SCRIPT_INTEROP,      // C++ createObject + callMethod on @Script classes
        NATIVE_CALLBACK,     // Pure C++ callback exercising ScriptAPI directly (MYT-42)
        EXE_TEST,            // Build .mtproj to standalone exe, run, verify output (MYT-62)
        EXE_GUI_TEST,        // As EXE_TEST, but built with --gui (windowed-subsystem launcher, MYT-326)
        DIRECT_SCRIPT_WITH_PROJECT  // MYT-310: run .mt directly, auto-discover ambient .mtproj
    };
}
