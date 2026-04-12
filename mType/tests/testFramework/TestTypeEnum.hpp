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
        EXE_TEST             // Build .mtproj to standalone exe, run, verify output (MYT-62)
    };
}
