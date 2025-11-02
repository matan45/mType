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

        // ====================================
        // NEW EDGE CASE TESTS (70 tests)
        // ====================================

        // === GENERIC TESTS (10 tests) ===
        addOutputVerificationTest("Type Generic Method Inference",
                        passPath + "typeGenericMethodInference_pass.mt");
        addOutputVerificationTest("Type Wildcard Upper Bound",
                        passPath + "typeWildcardUpperBound_pass.mt");
        addOutputVerificationTest("Type Wildcard Lower Bound",
                        passPath + "typeWildcardLowerBound_pass.mt");
        addOutputVerificationTest("Type Multiple Bounds",
                        passPath + "typeMultipleBounds_pass.mt");
        addOutputVerificationTest("Type Generic Inheritance",
                        passPath + "typeGenericInheritance_pass.mt");
        addOutputVerificationTest("Type Recursive Bound",
                        passPath + "typeRecursiveBound_pass.mt");
        addOutputVerificationTest("Type Nested Generic",
                        passPath + "typeNestedGeneric_pass.mt");
        addOutputVerificationTest("Type Generic Array",
                        passPath + "typeGenericArray_pass.mt");
        addTestFromFile("Type Generic Erasure Conflict",
                        errorPath + "typeGenericErasureConflict_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Type Generic Bridge Method",
                        passPath + "typeGenericBridgeMethod_pass.mt");

        // === INTERFACE TESTS (8 tests) ===
        addOutputVerificationTest("Type Interface Implementation",
                        passPath + "typeInterfaceImplementation_pass.mt");
        addOutputVerificationTest("Type Multiple Interfaces",
                        passPath + "typeMultipleInterfaces_pass.mt");
        addOutputVerificationTest("Type Interface Covariant Return",
                        passPath + "typeInterfaceCovariantReturn_pass.mt");
        addOutputVerificationTest("Type Interface Default Method",
                        passPath + "typeInterfaceDefaultMethod_pass.mt");
        addTestFromFile("Type Interface Missing Method",
                        errorPath + "typeInterfaceMissingMethod_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Interface Contravariant Param",
                        errorPath + "typeInterfaceContravariantParam_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Type Diamond Problem",
                        passPath + "typeDiamondProblem_pass.mt");
        addOutputVerificationTest("Type Interface Generic",
                        passPath + "typeInterfaceGeneric_pass.mt");

        // === INHERITANCE TESTS (8 tests) ===
        addOutputVerificationTest("Type Upcast",
                        passPath + "typeUpcast_pass.mt");
        addOutputVerificationTest("Type Downcast With Check",
                        passPath + "typeDowncastWithCheck_pass.mt");
        addTestFromFile("Type Downcast Without Check",
                        errorPath + "typeDowncastWithoutCheck_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Type Method Override Covariant",
                        passPath + "typeMethodOverrideCovariant_pass.mt");
        addTestFromFile("Type Method Override Contravariant",
                        errorPath + "typeMethodOverrideContravariant_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Type Abstract Method",
                        passPath + "typeAbstractMethod_pass.mt");
        addTestFromFile("Type Final Method Override",
                        errorPath + "typeFinalMethodOverride_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Type Protected Access",
                        passPath + "typeProtectedAccess_pass.mt");

        // === ARRAY TESTS (6 tests) ===
        addOutputVerificationTest("Type Array Covariance Safe",
                        passPath + "typeArrayCovarianceSafe_pass.mt");
        addTestFromFile("Type Array Store Exception",
                        errorPath + "typeArrayStoreException_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Type Multidimensional Array",
                        passPath + "typeMultidimensionalArray_pass.mt");
        addOutputVerificationTest("Type Array Element Assignment",
                        passPath + "typeArrayElementAssignment_pass.mt");
        addTestFromFile("Type Array Dimension Mismatch",
                        errorPath + "typeArrayDimensionMismatch_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Type Jagged Array",
                        passPath + "typeJaggedArray_pass.mt");

        // === LAMBDA TESTS (7 tests) ===
        addOutputVerificationTest("Type Lambda Parameter Inference",
                        passPath + "typeLambdaParameterInference_pass.mt");
        addOutputVerificationTest("Type Lambda Return Inference",
                        passPath + "typeLambdaReturnInference_pass.mt");
        addOutputVerificationTest("Type Lambda Capture",
                        passPath + "typeLambdaCapture_pass.mt");
        addTestFromFile("Type Lambda Capture Mutable",
                        errorPath + "typeLambdaCaptureMutable_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Type Method Reference",
                        passPath + "typeMethodReference_pass.mt");
        addOutputVerificationTest("Type Lambda Generic",
                        passPath + "typeLambdaGeneric_pass.mt");
        addOutputVerificationTest("Type Lambda Closure",
                        passPath + "typeLambdaClosure_pass.mt");

        // === NULL SAFETY TESTS (6 tests) ===
        addOutputVerificationTest("Type Nullable Generic",
                        passPath + "typeNullableGeneric_pass.mt");
        addOutputVerificationTest("Type Null Conditional",
                        passPath + "typeNullConditional_pass.mt");
        addOutputVerificationTest("Type Null Coalescing",
                        passPath + "typeNullCoalescing_pass.mt");
        addTestFromFile("Type Null Dereference",
                        errorPath + "typeNullDereference_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Type Nullable Flow Analysis",
                        passPath + "typeNullableFlowAnalysis_pass.mt");
        addOutputVerificationTest("Type Null Safe Cast",
                        passPath + "typeNullSafeCast_pass.mt");

        // === OPERATOR TESTS (5 tests) ===
        addOutputVerificationTest("Type Arithmetic Promotion",
                        passPath + "typeArithmeticPromotion_pass.mt");
        addTestFromFile("Type Comparison String Int",
                        errorPath + "typeComparisonStringInt_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Type Compound Assignment",
                        passPath + "typeCompoundAssignment_pass.mt");
        addOutputVerificationTest("Type Ternary Type Inference",
                        passPath + "typeTernaryTypeInference_pass.mt");
        addOutputVerificationTest("Type Operator Overload",
                        passPath + "typeOperatorOverload_pass.mt");

        // === ADVANCED FEATURES (8 tests) ===
        addOutputVerificationTest("Type Type Guard",
                        passPath + "typeTypeGuard_pass.mt");
        addOutputVerificationTest("Type Structural Subtyping",
                        passPath + "typeStructuralSubtyping_pass.mt");
        addOutputVerificationTest("Type Nominal Typing",
                        passPath + "typeNominalTyping_pass.mt");
        addOutputVerificationTest("Type Variance Annotation",
                        passPath + "typeVarianceAnnotation_pass.mt");
        addOutputVerificationTest("Type Flow Sensitive",
                        passPath + "typeFlowSensitive_pass.mt");
        addOutputVerificationTest("Type Union Type",
                        passPath + "typeUnionType_pass.mt");
        addOutputVerificationTest("Type Intersection Type",
                        passPath + "typeIntersectionType_pass.mt");
        addTestFromFile("Type Cyclic Inheritance",
                        errorPath + "typeCyclicInheritance_error.mt",
                        TestType::ERROR_EXPECTED);

        // === OVERLOADING TESTS (6 tests) ===
        addOutputVerificationTest("Type Method Overload",
                        passPath + "typeMethodOverload_pass.mt");
        addOutputVerificationTest("Type Constructor Overload",
                        passPath + "typeConstructorOverload_pass.mt");
        addTestFromFile("Type Overload Ambiguous",
                        errorPath + "typeOverloadAmbiguous_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Type Generic Overload",
                        passPath + "typeGenericOverload_pass.mt");
        addOutputVerificationTest("Type Varargs Overload",
                        passPath + "typeVarargsOverload_pass.mt");
        addOutputVerificationTest("Type Overload Resolution",
                        passPath + "typeOverloadResolution_pass.mt");

        // === BOUNDS TESTS (5 tests) ===
        addOutputVerificationTest("Type Upper Bound",
                        passPath + "typeUpperBound_pass.mt");
        addOutputVerificationTest("Type Lower Bound",
                        passPath + "typeLowerBound_pass.mt");
        addOutputVerificationTest("Type Multiple Constraints",
                        passPath + "typeMultipleConstraints_pass.mt");
        addOutputVerificationTest("Type Recursive Bound Complex",
                        passPath + "typeRecursiveBoundComplex_pass.mt");
        addTestFromFile("Type Bound Violation",
                        errorPath + "typeBoundViolation_error.mt",
                        TestType::ERROR_EXPECTED);

        // === INFERENCE TESTS (5 tests) ===
        addOutputVerificationTest("Type Local Variable Inference",
                        passPath + "typeLocalVariableInference_pass.mt");
        addOutputVerificationTest("Type Generic Method Call Inference",
                        passPath + "typeGenericMethodCallInference_pass.mt");
        addOutputVerificationTest("Type Lambda Expression Inference",
                        passPath + "typeLambdaExpressionInference_pass.mt");
        addOutputVerificationTest("Type Conditional Expression Inference",
                        passPath + "typeConditionalExprInference_pass.mt");
        addOutputVerificationTest("Type Array Initializer Inference",
                        passPath + "typeArrayInitializerInference_pass.mt");

        // === MIXED-MODE TESTS (5 tests) ===
        addTestFromFile("Type Void Assignment",
                        errorPath + "typeVoidAssignment_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Circular Dependency",
                        errorPath + "typeCircularDependency_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Type Primitive Wrapper",
                        passPath + "typePrimitiveWrapper_pass.mt");
        addOutputVerificationTest("Type Reflection",
                        passPath + "typeReflection_pass.mt");
        addOutputVerificationTest("Type Dynamic Dispatch",
                        passPath + "typeDynamicDispatch_pass.mt");

        // === ERROR MESSAGES (1 test) ===
        addTestFromFile("Type Cascading Error",
                        errorPath + "typeCascadingError_error.mt",
                        TestType::ERROR_EXPECTED);

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
