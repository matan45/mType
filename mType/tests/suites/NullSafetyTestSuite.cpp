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

        // === NULLABLE FUNCTION PARAMS & RETURN TYPES ===
        addOutputVerificationTest("Nullable Function Params",
                        passPath + "nullableFunctionParams.mt");

        // === NULLABLE VALUE OBJECTS ===
        addOutputVerificationTest("Nullable Value Object",
                        passPath + "nullableValueObject.mt");

        // === JIT NULL CHECK ELIMINATION ===
        addOutputVerificationTest("JIT Null Check Elimination",
                        passPath + "jitNullCheckElimination.mt");

        // === ERROR TESTS ===
        addTestFromFile("Null to Non-Nullable Assignment",
                        errorPath + "nullToNonNullable_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nullable Primitive Type",
                        errorPath + "nullablePrimitiveType_error.mt",
                        TestType::ERROR_EXPECTED);
    }
}
