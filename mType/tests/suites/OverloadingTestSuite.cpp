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
        // Tests for overload resolution error scenarios.
        // Substring matchers pin which error fires - without them a regression that
        // swaps "ambiguous" for "no matching overload" (or any other throw) would
        // pass silently.

        addTestFromFile("Ambiguous Call",
                    errorPath + "ambiguousCall.mt",
                    TestType::ERROR_EXPECTED,
                    "ambiguous call to");
        addTestFromFile("No Matching Overload",
                    errorPath + "noMatchingOverload.mt",
                    TestType::ERROR_EXPECTED,
                    "no matching overload for call to");
        addTestFromFile("Null Ambiguity Two Classes",
                    errorPath + "nullAmbiguityTwoClasses.mt",
                    TestType::ERROR_EXPECTED,
                    "ambiguous call to");

        // Duplicate-signature errors (registration phase, MT-E0004).
        // DuplicateSignatureException::formatMessage produces:
        //   "Duplicate <type> signature: '<name>(<sig>)' has already been declared at <loc>"
        // so the per-test substring is the type-specific phrase.
        addTestFromFile("Duplicate Method Signature",
                    errorPath + "duplicateMethodSignature.mt",
                    TestType::ERROR_EXPECTED,
                    "Duplicate instance method declaration");
        addTestFromFile("Duplicate Static Method Signature",
                    errorPath + "duplicateStaticMethodSignature.mt",
                    TestType::ERROR_EXPECTED,
                    "Duplicate static method declaration");
        addTestFromFile("Return Type Only Overload",
                    errorPath + "returnTypeOnlyOverload.mt",
                    TestType::ERROR_EXPECTED,
                    "Duplicate instance method declaration");

        // Overload resolution errors at call sites.
        addTestFromFile("Null Ambiguity Inheritance",
                    errorPath + "nullAmbiguityInheritance.mt",
                    TestType::ERROR_EXPECTED,
                    "ambiguous call to");
        addTestFromFile("Constructor No Matching Overload",
                    errorPath + "constructorNoMatchingOverload.mt",
                    TestType::ERROR_EXPECTED,
                    "no matching overload for call to");
        addTestFromFile("Diamond Interface Ambiguous",
                    errorPath + "diamondInterfaceAmbiguous.mt",
                    TestType::ERROR_EXPECTED,
                    "ambiguous call to");

        // === EDGE CASE TESTS - static vs instance same name / generic vs concrete / Int-Float ambig ===
        addOutputVerificationTest("Overload Static And Instance Same Name",
                        passPath + "overloadStaticAndInstanceSameName.mt");
        addOutputVerificationTest("Overload Generic Vs Concrete",
                        passPath + "overloadGenericVsConcrete.mt");

        addOutputVerificationTest("Overload Ambiguous Int Float",
                        passPath + "overloadAmbiguousIntFloat.mt");
        addOutputVerificationTest("Static Overload Return Type Recovery",
                        passPath + "staticOverloadReturnsVoid.mt");
        addOutputVerificationTest("Static Overload Chained Member Access",
                        passPath + "overloadStaticChainedMemberAccess.mt");
        addOutputVerificationTest("Static Overload Assigned To Var",
                        passPath + "overloadStaticAssignedToVar.mt");
        addOutputVerificationTest("Static Overload In Arithmetic",
                        passPath + "overloadStaticInArithmetic.mt");
        addOutputVerificationTest("Static Overload Nested As Arg",
                        passPath + "overloadStaticNestedAsArg.mt");
    }
}
