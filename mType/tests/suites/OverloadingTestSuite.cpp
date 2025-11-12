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

        // === ERROR TESTS ===
        // Tests for overload resolution error scenarios

        addTestFromFile("Ambiguous Call",
                    errorPath + "ambiguousCall.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("No Matching Overload",
                    errorPath + "noMatchingOverload.mt",
                    TestType::ERROR_EXPECTED);
    }
}
