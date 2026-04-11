#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    /**
     * MYT-42 — first suite in the project that asserts on value::Value
     * results directly from C++ via ScriptAPI. Every test is a
     * NATIVE_CALLBACK — no .mt file is driven through stdout+.expected.
     *
     * The callbacks exercise the new FFI surface (getClass,
     * getGenericArguments, classOf, isInstanceOf) plus the hygiene
     * contract that ObjectInstance must never need to be unwrapped in
     * native user code.
     */
    class ScriptApiNativeTestSuite : public testFramework::TestSuite
    {
    public:
        explicit ScriptApiNativeTestSuite()
            : TestSuite("ScriptAPI Native Test Suite") {}
        void setupTests() override;
    };
}
