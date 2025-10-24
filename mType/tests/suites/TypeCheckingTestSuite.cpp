#include "TypeCheckingTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void TypeCheckingTestSuite::setupTests()
    {
        // Object assignment tests
        addOutputVerificationTest("Same Type Assignment",
                        passPath + "sameTypeAssignment.mt");
        addOutputVerificationTest("Null Assignment",
                        passPath + "nullAssignment.mt");
        addOutputVerificationTest("Multiple Assignments",
                        passPath + "multipleAssignments.mt");
        addOutputVerificationTest("Complex Type Checking",
                        passPath + "complexTypeChecking.mt");
        addOutputVerificationTest("Declaration Type Checking",
                        passPath + "declarationTypeChecking.mt");
        addOutputVerificationTest("Declaration Type Checking Mixed",
                        passPath + "declarationTypeCheckingMixed.mt");
        addOutputVerificationTest("Chained Object Assignments",
                        passPath + "chainedObjectAssignments.mt");
        addOutputVerificationTest("Complex Reassignment Chains",
                        passPath + "complexReassignmentChains.mt");

        // Function parameter tests
        addOutputVerificationTest("Function Parameter Primitive Types",
                        passPath + "functionParameterPrimitiveTypes.mt");
        addOutputVerificationTest("Function Parameter Object Types",
                        passPath + "functionParameterObjectTypes.mt");
        addOutputVerificationTest("Function Parameter Null Values",
                        passPath + "functionParameterNullValues.mt");
        addOutputVerificationTest("Function Parameter Float Conversion",
                        passPath + "functionParameterFloatConversion.mt");
        addOutputVerificationTest("Method Parameter Type Checking",
                        passPath + "methodParameterTypeChecking.mt");
        addOutputVerificationTest("Constructor Parameter Type Checking",
                        passPath + "constructorParameterTypeChecking.mt");

        // Function return type tests
        addOutputVerificationTest("Function Return Type Matching",
                        passPath + "functionReturnTypeMatching.mt");
        addOutputVerificationTest("Object Return Types",
                        passPath + "objectReturnTypes.mt");
        addOutputVerificationTest("Object Return Types Function Returning Object",
                        passPath + "objectReturnTypesFunctionReturningObject.mt");
        addOutputVerificationTest("Object Return Types Static Methods",
                        passPath + "objectReturnTypesStaticMethods.mt");
        addOutputVerificationTest("Object Return Types Nested Objects",
                        passPath + "objectReturnTypesNestedObjects.mt");

        // String handling edge case tests
        addOutputVerificationTest("String Concatenation with Null Objects",
                        passPath + "stringConcatenationWithNulls.mt");
        addOutputVerificationTest("Empty String Edge Cases",
                        passPath + "emptyStringEdgeCases.mt");

        // Null propagation tests
        addOutputVerificationTest("Null Propagation Complex Expressions",
                        passPath + "nullPropagationComplexExpressions.mt");

        // === NEW EDGE CASE TESTS ===
        // Advanced type system scenarios

        addOutputVerificationTest("Nullable Chain Safe",
                        passPath + "nullableChainSafe.mt");

        // Error tests (expected to fail)
        addTestFromFile("Different Type Assignment Error",
                        errorPath + "differentTypeAssignmentError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Complex Type Checking Invalid Assignment",
                        errorPath + "complexTypeCheckingInvalidAssignment.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Declaration Type Checking Invalid",
                        errorPath + "declarationTypeCheckingInvalid.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Declaration Type Checking Mixed Invalid",
                        errorPath + "declarationTypeCheckingMixedInvalid.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Function Parameter Type Mismatch Int For String",
                        errorPath + "functionParameterTypeMismatchIntForString.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Function Parameter Type Mismatch String For Int",
                        errorPath + "functionParameterTypeMismatchStringForInt.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Function Parameter Type Mismatch Bool For Object",
                        errorPath + "functionParameterTypeMismatchBoolForObject.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Function Parameter Wrong Order",
                        errorPath + "functionParameterWrongOrder.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Function Parameter Float Conversion String Error",
                        errorPath + "functionParameterFloatConversionStringError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Method Parameter Type Checking Wrong",
                        errorPath + "methodParameterTypeCheckingWrong.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Constructor Parameter Type Checking Wrong Order",
                        errorPath + "constructorParameterTypeCheckingWrongOrder.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Function Return Type Mismatch String To Bool",
                        errorPath + "functionReturnTypeMismatchStringToBool.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Function Return Type Mismatch Int To String",
                        errorPath + "functionReturnTypeMismatchIntToString.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Function Return Type Mismatch Float To Bool",
                        errorPath + "functionReturnTypeMismatchFloatToBool.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Function Return Type Mismatch Bool To String",
                        errorPath + "functionReturnTypeMismatchBoolToString.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Function Return Type Mismatch Method Int To String",
                        errorPath + "functionReturnTypeMismatchMethodIntToString.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Object Return Types Method Mismatch",
                        errorPath + "objectReturnTypesMethodMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Object Return Types Wrong Object Type",
                        errorPath + "objectReturnTypesWrongObjectType.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Object Return Types Static Method Mismatch",
                        errorPath + "objectReturnTypesStaticMethodMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Reassignment Chains",
                        errorPath + "invalidReassignmentChains.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Mixed Type Chain Assignment Error",
                        errorPath + "mixedTypeChainAssignmentError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Conversion Failure Boundary Tests",
                        errorPath + "typeConversionFailureBoundaryTests.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Boolean Conversion Boundary Error",
                        errorPath + "booleanConversionBoundaryError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Numeric Overflow Boundary Error",
                        errorPath + "numericOverflowBoundaryError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Array To Scalar Conversion Error",
                        errorPath + "arrayToScalarConversionError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Null To Non-Nullable Conversion Error",
                        errorPath + "nullToNonNullableConversionError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Function Type Conversion Boundary Error",
                        errorPath + "functionTypeConversionBoundaryError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Incompatible Class Conversion Error",
                        errorPath + "incompatibleClassConversionError.mt",
                        TestType::ERROR_EXPECTED);

        // Naming convention error tests
        addTestFromFile("Function Naming Convention Error",
                        errorPath + "functionNamingConventionError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Method Naming Convention Error",
                        errorPath + "methodNamingConventionError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Method Naming Convention Error",
                        errorPath + "staticMethodNamingConventionError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Native Function Naming Convention Error",
                        errorPath + "nativeFunctionNamingConventionError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Mixed Naming Convention Errors",
                        errorPath + "mixedNamingConventionErrors.mt",
                        TestType::ERROR_EXPECTED);

        // === NEW EDGE CASE ERROR TESTS ===
        // Advanced type system error scenarios

        addTestFromFile("Float To Int Conversion Error",
                        errorPath + "floatToIntConversion.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Array Covariance Error",
                        errorPath + "arrayCovariance.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Lambda Contravariance Error",
                        errorPath + "lambdaContravariance.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Inference Ambiguity Error",
                        errorPath + "typeInferenceAmbiguity.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Boolean To Numeric Conversion Error",
                        errorPath + "booleanToNumericError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Numeric Overflow Error",
                        errorPath + "numericOverflowError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("String To Numeric Conversion Error",
                        errorPath + "stringToNumericError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Mixed Type Comparison Error",
                        errorPath + "mixedTypeComparisonError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Jagged Array Mismatch Error",
                        errorPath + "jaggedArrayMismatch.mt",
                        TestType::ERROR_EXPECTED);
    }
}
