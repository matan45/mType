#include "NullSafetyTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void NullSafetyTestSuite::setupTests()
    {
        // === NULLABLE TYPE BASICS ===
        addOutputVerificationTest("Nullable Type Basic",
                        passPath + "nullableTypeBasic.mt");

        // === SMART CAST (NULL NARROWING) ===
        addOutputVerificationTest("Smart Cast Basic",
                        passPath + "smartCastBasic.mt");

        // === ERROR TESTS ===
        addTestFromFile("Null to Non-Nullable Assignment",
                        errorPath + "nullToNonNullable_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nullable Primitive Type",
                        errorPath + "nullablePrimitiveType_error.mt",
                        TestType::ERROR_EXPECTED);
    }
}
