#include "OverloadingTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void OverloadingTestSuite::setupTests()
    {
        // === BASIC OVERLOADING TESTS ===
        // Tests for fundamental overload resolution features

        addOutputVerificationTest("Basic Function Overload",
                        passPath + "basicFunctionOverload.mt");
        addOutputVerificationTest("Basic Method Overload",
                        passPath + "basicMethodOverload.mt");
        addOutputVerificationTest("Static Method Overload",
                        passPath + "staticMethodOverload.mt");
        addOutputVerificationTest("Constructor Overload",
                        passPath + "constructorOverload.mt");

        // === GENERIC OVERLOADING TESTS ===
        // Tests for overloading with generic type parameters

        addOutputVerificationTest("Generic Function Overload",
                        passPath + "genericFunctionOverload.mt");
        addOutputVerificationTest("Generic Method Overload",
                        passPath + "genericMethodOverload.mt");

        // === INTEGRATION TESTS ===
        // Comprehensive tests combining multiple overloading scenarios

        addOutputVerificationTest("Integration Test",
                        passPath + "integrationTest.mt");

        // === ADVANCED TESTS ===
        // Complex nested generics and edge case scenarios

        addOutputVerificationTest("Nested Generics Overload",
                        passPath + "nestedGenericsOverload.mt");
        addOutputVerificationTest("Edge Cases Overload",
                        passPath + "edgeCasesOverload.mt");
        addOutputVerificationTest("Complex Overload Scenarios",
                        passPath + "complexOverloadScenarios.mt");

        // === EDGE CASE TESTS ===
        // Tests targeting conversion priority boundaries and tricky resolution scenarios

        addOutputVerificationTest("Deep Inheritance Distance",
                        passPath + "deepInheritanceDistance.mt");
        addOutputVerificationTest("Widening vs AutoBoxing",
                        passPath + "wideningVsAutoBoxing.mt");
        addOutputVerificationTest("Three Way Conversion Priority",
                        passPath + "threeWayConversionPriority.mt");
        addOutputVerificationTest("Interface vs Class Overload",
                        passPath + "interfaceVsClassOverload.mt");
        addOutputVerificationTest("Constructor Overload Widening",
                        passPath + "constructorOverloadWidening.mt");
        addOutputVerificationTest("Zero vs One Param",
                        passPath + "zeroVsOneParam.mt");
        addOutputVerificationTest("Single Widening Fallback",
                        passPath + "singleWideningFallback.mt");
        addOutputVerificationTest("Generic vs Concrete Preference",
                        passPath + "genericVsConcretePreference.mt");
        addOutputVerificationTest("Multi Param Mixed Conversions",
                        passPath + "multiParamMixedConversions.mt");
        addOutputVerificationTest("Null With Primitive Overload",
                        passPath + "nullWithPrimitiveOverload.mt");

        // === CROSS-FEATURE EDGE CASES ===
        addOutputVerificationTest("Overload Generic + Nullable",
                        passPath + "overloadGenericNullable.mt");

        // === ERROR TESTS ===
        // Tests for overload resolution error scenarios

        addTestFromFile("Ambiguous Call",
                    errorPath + "ambiguousCall.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("No Matching Overload",
                    errorPath + "noMatchingOverload.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Null Ambiguity Two Classes",
                    errorPath + "nullAmbiguityTwoClasses.mt",
                    TestType::ERROR_EXPECTED);
    }
}
