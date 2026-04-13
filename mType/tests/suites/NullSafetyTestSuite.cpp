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

        // === NULL SAFE PARAM AND RETURN ===
        addOutputVerificationTest("Null Safe Param and Return",
                        passPath + "nullSafeParamAndReturn.mt");

        // === ERROR TESTS ===
        addTestFromFile("Null to Non-Nullable Assignment",
                        errorPath + "nullToNonNullable_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nullable Primitive Type",
                        errorPath + "nullablePrimitiveType_error.mt",
                        TestType::ERROR_EXPECTED);

        // === COMPILE-TIME NULL SAFETY ENFORCEMENT ===
        addTestFromFile("Member Access on Nullable Receiver",
                        errorPath + "memberAccessOnNullable_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Method Call on Nullable Receiver",
                        errorPath + "methodCallOnNullable_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Member Assignment on Nullable Receiver",
                        errorPath + "memberAssignOnNullable_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Chained Access Through Nullable Field",
                        errorPath + "chainedAccessThroughNullable_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Null Arg to Non-Null Parameter",
                        errorPath + "nullArgToNonNullParam_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nullable Arg to Non-Null Parameter",
                        errorPath + "nullableArgToNonNullParam_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Null Arg to Non-Null Constructor Parameter",
                        errorPath + "nullArgToNonNullCtorParam_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Return Null from Non-Null Function",
                        errorPath + "returnNullFromNonNull_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nullable Parameter Not Narrowed",
                        errorPath + "nullableParamNotNarrowed_error.mt",
                        TestType::ERROR_EXPECTED);
    }
}
