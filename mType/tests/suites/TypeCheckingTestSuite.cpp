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
        // ADDITIONAL TESTS - Files that exist but weren't registered
        // ====================================

        // === GENERIC TYPE TESTS (16 tests) ===
        addOutputVerificationTest("Type Array Of Generics",
                        passPath + "typeArrayOfGenerics_pass.mt");
        addOutputVerificationTest("Type Array Rectangular Vs Jagged",
                        passPath + "typeArrayRectangularVsJagged_pass.mt");
        addOutputVerificationTest("Type Bounds Propagation",
                        passPath + "typeBoundsPropagation_pass.mt");
        addOutputVerificationTest("Type Bounds Recursive",
                        passPath + "typeBoundsRecursive_pass.mt");
        addOutputVerificationTest("Type Generic Class Instantiation",
                        passPath + "typeGenericClassInstantiation_pass.mt");
        addOutputVerificationTest("Type Generic Erasure",
                        passPath + "typeGenericErasure_pass.mt");
        addOutputVerificationTest("Type Generic Method Inference",
                        passPath + "typeGenericMethodInference_pass.mt");
        addOutputVerificationTest("Type Generic Multiple Bounds",
                        passPath + "typeGenericMultipleBounds_pass.mt");
        addOutputVerificationTest("Type Generic Nested Instantiation",
                        passPath + "typeGenericNestedInstantiation_pass.mt");
        addOutputVerificationTest("Type Generic Self Referencing",
                        passPath + "typeGenericSelfReferencing_pass.mt");
        addOutputVerificationTest("Type Generic Wildcard Lower",
                        passPath + "typeGenericWildcardLower_pass.mt");
        addOutputVerificationTest("Type Generic Wildcard Upper",
                        passPath + "typeGenericWildcardUpper_pass.mt");
        addOutputVerificationTest("Type Array Multidimensional",
                        passPath + "typeArrayMultidimensional_pass.mt");
        addOutputVerificationTest("Type Dependent Type",
                        passPath + "typeDependentType_pass.mt");

        // === INTERFACE & INHERITANCE TESTS (9 tests) ===
        addOutputVerificationTest("Type Interface Covariant Return",
                        passPath + "typeInterfaceCovariantReturn_pass.mt");
        addOutputVerificationTest("Type Interface Diamond Problem",
                        passPath + "typeInterfaceDiamondProblem_pass.mt");
        addOutputVerificationTest("Type Interface Generic Method",
                        passPath + "typeInterfaceGenericMethod_pass.mt");
        addOutputVerificationTest("Type Interface Implementation",
                        passPath + "typeInterfaceImplementation_pass.mt");
        addOutputVerificationTest("Type Interface Segregation",
                        passPath + "typeInterfaceSegregation_pass.mt");
        addOutputVerificationTest("Type Inheritance Downcast",
                        passPath + "typeInheritanceDowncast_pass.mt");
        addOutputVerificationTest("Type Inheritance Method Override Covariant",
                        passPath + "typeInheritanceMethodOverrideCovariant_pass.mt");
        addOutputVerificationTest("Type Inheritance Upcast",
                        passPath + "typeInheritanceUpcast_pass.mt");
        addOutputVerificationTest("Type Inheritance Virtual Dispatch",
                        passPath + "typeInheritanceVirtualDispatch_pass.mt");

        // === LAMBDA TESTS (6 tests) ===
        addOutputVerificationTest("Type Lambda Closure",
                        passPath + "typeLambdaClosure_pass.mt");
        addOutputVerificationTest("Type Lambda Functional Interface",
                        passPath + "typeLambdaFunctionalInterface_pass.mt");
        addOutputVerificationTest("Type Lambda Higher Order",
                        passPath + "typeLambdaHigherOrder_pass.mt");
        addOutputVerificationTest("Type Lambda Method Reference",
                        passPath + "typeLambdaMethodReference_pass.mt");
        addOutputVerificationTest("Type Lambda Parameter Inference",
                        passPath + "typeLambdaParameterInference_pass.mt");

        // === NULL SAFETY TESTS (6 tests) ===
        addOutputVerificationTest("Type Nullable Generic",
                        passPath + "typeNullableGeneric_pass.mt");
        addOutputVerificationTest("Type Null Assertion",
                        passPath + "typeNullAssertion_pass.mt");
        addOutputVerificationTest("Type Null Coalescing",
                        passPath + "typeNullCoalescing_pass.mt");
        addOutputVerificationTest("Type Null Conditional",
                        passPath + "typeNullConditional_pass.mt");
        addOutputVerificationTest("Type Null Propagation",
                        passPath + "typeNullPropagation_pass.mt");

        // === TYPE INFERENCE TESTS (5 tests) ===
        addOutputVerificationTest("Type Inference Collection Literal",
                        passPath + "typeInferenceCollectionLiteral_pass.mt");
        addOutputVerificationTest("Type Inference Conditional",
                        passPath + "typeInferenceConditional_pass.mt");
        addOutputVerificationTest("Type Inference Generic Method",
                        passPath + "typeInferenceGenericMethod_pass.mt");
        addOutputVerificationTest("Type Inference Lambda Return",
                        passPath + "typeInferenceLambdaReturn_pass.mt");
        addOutputVerificationTest("Type Inference Local Variable",
                        passPath + "typeInferenceLocalVariable_pass.mt");

        // === OPERATOR TESTS (3 tests) ===
        addOutputVerificationTest("Type Operator Arithmetic Promotion",
                        passPath + "typeOperatorArithmeticPromotion_pass.mt");
        addOutputVerificationTest("Type Operator Assignment Compound",
                        passPath + "typeOperatorAssignmentCompound_pass.mt");
        addOutputVerificationTest("Type Operator Logical Short Circuit",
                        passPath + "typeOperatorLogicalShortCircuit_pass.mt");

        // === OVERLOADING TESTS (5 tests) ===
        addOutputVerificationTest("Type Overload Constructor",
                        passPath + "typeOverloadConstructor_pass.mt");
        addOutputVerificationTest("Type Overload Extension",
                        passPath + "typeOverloadExtension_pass.mt");
        addOutputVerificationTest("Type Overload Generic",
                        passPath + "typeOverloadGeneric_pass.mt");
        addOutputVerificationTest("Type Overload Static Hiding",
                        passPath + "typeOverloadStaticHiding_pass.mt");
        addOutputVerificationTest("Type Overload Varargs",
                        passPath + "typeOverloadVarargs_pass.mt");

        // === ADVANCED TYPE FEATURES (8 tests) ===
        addOutputVerificationTest("Type Flow Sensitive",
                        passPath + "typeFlowSensitive_pass.mt");
        addOutputVerificationTest("Type Intersection Type",
                        passPath + "typeIntersectionType_pass.mt");
        addOutputVerificationTest("Type Structural Typing",
                        passPath + "typeStructuralTyping_pass.mt");
        addOutputVerificationTest("Type Type Guard",
                        passPath + "typeTypeGuard_pass.mt");
        addOutputVerificationTest("Type Union Type",
                        passPath + "typeUnionType_pass.mt");
        addOutputVerificationTest("Type Variance Contravariant",
                        passPath + "typeVarianceContravariant_pass.mt");
        addOutputVerificationTest("Type Variance Covariant",
                        passPath + "typeVarianceCovariant_pass.mt");

        // === MIXED-MODE TESTS (4 tests) ===
        addOutputVerificationTest("Type Mixed Circular Dependency",
                        passPath + "typeMixedCircularDependency_pass.mt");
        addOutputVerificationTest("Type Mixed Native Method",
                        passPath + "typeMixedNativeMethod_pass.mt");
        addOutputVerificationTest("Type Mixed Reflection",
                        passPath + "typeMixedReflection_pass.mt");
        addOutputVerificationTest("Type Mixed Serialization",
                        passPath + "typeMixedSerialization_pass.mt");

        // === ADDITIONAL ERROR TESTS (19 tests) ===
        addTestFromFile("Type Array Covariance Error",
                        errorPath + "typeArrayCovariance_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Array Element Assignment Error",
                        errorPath + "typeArrayElementAssignment_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Array Length Boundary Error",
                        errorPath + "typeArrayLengthBoundary_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Bounds Lower Violation Error",
                        errorPath + "typeBoundsLowerViolation_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Bounds Multiple Violation Error",
                        errorPath + "typeBoundsMultipleViolation_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Bounds Upper Violation Error",
                        errorPath + "typeBoundsUpperViolation_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Error Message Detailed Error",
                        errorPath + "typeErrorMessageDetailed_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Generic Array Creation Error",
                        errorPath + "typeGenericArrayCreation_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Inheritance Abstract Method Error",
                        errorPath + "typeInheritanceAbstractMethod_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Inheritance Final Override Error",
                        errorPath + "typeInheritanceFinalOverride_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Inheritance Method Override Invariant Error",
                        errorPath + "typeInheritanceMethodOverrideInvariant_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Inheritance Protected Access Error",
                        errorPath + "typeInheritanceProtectedAccess_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Interface Contravariant Param Error",
                        errorPath + "typeInterfaceContravariantParam_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Interface Method Signature Error",
                        errorPath + "typeInterfaceMethodSignature_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Interface Multiple Conflict Error",
                        errorPath + "typeInterfaceMultipleConflict_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Lambda Exception Error",
                        errorPath + "typeLambdaException_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Lambda Return Type Mismatch Error",
                        errorPath + "typeLambdaReturnTypeMismatch_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Mixed Void Misuse Error",
                        errorPath + "typeMixedVoidMisuse_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Nullable Primitive Error",
                        errorPath + "typeNullablePrimitive_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Operator Bitwise Error",
                        errorPath + "typeOperatorBitwise_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Operator Comparison Error",
                        errorPath + "typeOperatorComparison_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Overload Ambiguous Error",
                        errorPath + "typeOverloadAmbiguous_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Namespace Return Types Mismatch Error",
                        errorPath + "namespaceReturnTypesMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Namespace Return Types Wrong Class Error",
                        errorPath + "namespaceReturnTypesWrongClass.mt",
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
