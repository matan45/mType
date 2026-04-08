#pragma once

namespace tests::testFramework
{
    enum class TestType {
        NORMAL,
        ERROR_EXPECTED,
        OUTPUT_EXPECTED,
        PERFORMANCE,
        COMPILATION_TEST,
        SCRIPT_INTEROP       // C++ createObject + callMethod on @Script classes
    };
}
