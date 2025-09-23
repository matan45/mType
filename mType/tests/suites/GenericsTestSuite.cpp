#include "GenericsTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void GenericsTestSuite::setupTests()
    {
        // Basic generic functionality tests
        addOutputVerificationTest("Basic Generic Class",
                        passPath + "basicGenericClass.mt");
        addOutputVerificationTest("Multiple Type Parameters",
                        passPath + "multipleTypeParameters.mt");
        addOutputVerificationTest("Generic with Primitive Types",
                        passPath + "genericWithPrimitiveTypes.mt");
        addOutputVerificationTest("Generic Constructors",
                        passPath + "genericConstructors.mt");
        addOutputVerificationTest("Generic Method Overloading",
                        passPath + "genericMethodOverloading.mt");
        addOutputVerificationTest("Generic Null Handling",
                        passPath + "genericNullHandling.mt");
        addOutputVerificationTest("Generic Import Test",
                        passPath + "genericImportTest.mt");
        addOutputVerificationTest("HashMap HashSet Optimization",
                        passPath + "hashmapHashsetOptimization.mt");

        // Static generic method tests
        addOutputVerificationTest("Static Generic Methods",
                        passPath + "staticGenericMethods.mt");
        addOutputVerificationTest("Static Generic Method Parameterless",
                        passPath + "staticGenericMethodParameterless.mt");
        addOutputVerificationTest("Mixed Static Generic Methods",
                        passPath + "mixedStaticGenericMethods.mt");
        addOutputVerificationTest("Static Generic Simple Returns",
                        passPath + "staticGenericSimpleReturns.mt");
        addOutputVerificationTest("Static Generic Simple Container",
                        passPath + "staticGenericSimpleContainer.mt");
        addOutputVerificationTest("Static Generic Collection Returns",
                        passPath + "staticGenericCollectionReturns.mt");

        // Error handling tests
        addTestFromFile("Invalid Type Argument Count",
                    errorPath + "invalidTypeArgumentCount.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Non-Generic with Type Args",
                    errorPath + "nonGenericWithTypeArgs.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Type Mismatch in Method",
                    errorPath + "typeMismatchInMethod.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Undefined Generic Class",
                    errorPath + "undefinedGenericClass.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Return Type Assignment",
                    errorPath + "invalidReturnTypeAssignment.mt",
                    TestType::ERROR_EXPECTED);

        // Static generic method error tests
        addTestFromFile("Static Generic Wrong Type Arg Count",
                    errorPath + "staticGenericWrongTypeArgCount.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Static Non-Generic with Type Args",
                    errorPath + "staticNonGenericWithTypeArgs.mt",
                    TestType::ERROR_EXPECTED);
    }
}