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

        // Stress tests for deeply nested generics
        addOutputVerificationTest("Deeply Nested Generics Stress Test",
                        passPath + "deeplyNestedGenericsStressTest.mt");
        addOutputVerificationTest("Generic Complexity Stress Test",
                        passPath + "genericComplexityStressTest.mt");

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

        // Global generic function tests
        addOutputVerificationTest("Global Generic Functions",
                        passPath + "globalGenericFunctions.mt");
        addOutputVerificationTest("Global Generic Multiple Parameters",
                        passPath + "globalGenericMultipleParams.mt");

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

        // Primitive type validation error tests
        addTestFromFile("Primitive String Type Rejected",
                    errorPath + "primitiveStringType.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Primitive Int Type Rejected",
                    errorPath + "primitiveIntType.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Primitive Float Type Rejected",
                    errorPath + "primitiveFloatType.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Primitive Bool Type Rejected",
                    errorPath + "primitiveBoolType.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Primitive Void Type Rejected",
                    errorPath + "primitiveVoidType.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Multiple Primitive Types Rejected",
                    errorPath + "multiplePrimitiveTypes.mt",
                    TestType::ERROR_EXPECTED);

        // Global generic function error tests
        addTestFromFile("Global Non-Generic with Type Args",
                    errorPath + "globalNonGenericWithTypeArgs.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Global Generic Wrong Type Arg Count",
                    errorPath + "globalGenericWrongTypeArgCount.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Global Generic Primitive Type",
                    errorPath + "globalGenericPrimitiveType.mt",
                    TestType::ERROR_EXPECTED);
    }
}