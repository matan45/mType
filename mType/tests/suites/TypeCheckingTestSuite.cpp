#include "TypeCheckingTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void TypeCheckingTestSuite::setupTests()
    {
        // Object assignment tests
        addTestFromFile("Same Type Assignment",
                        passPath + "sameTypeAssignment.mt");
        addTestFromFile("Null Assignment",
                        passPath + "nullAssignment.mt");
        addTestFromFile("Multiple Assignments",
                        passPath + "multipleAssignments.mt");
        addTestFromFile("Complex Type Checking",
                        passPath + "complexTypeChecking.mt");
        addTestFromFile("Declaration Type Checking",
                        passPath + "declarationTypeChecking.mt");
        addTestFromFile("Declaration Type Checking Mixed",
                        passPath + "declarationTypeCheckingMixed.mt");
        addTestFromFile("Chained Object Assignments",
                        passPath + "chainedObjectAssignments.mt");
        addTestFromFile("Complex Reassignment Chains",
                        passPath + "complexReassignmentChains.mt");
        addTestFromFile("Namespace Reassignment Chains",
                        passPath + "namespaceReassignmentChains.mt");

        // Function parameter tests
        addTestFromFile("Function Parameter Primitive Types",
                        passPath + "functionParameterPrimitiveTypes.mt");
        addTestFromFile("Function Parameter Object Types",
                        passPath + "functionParameterObjectTypes.mt");
        addTestFromFile("Function Parameter Null Values",
                        passPath + "functionParameterNullValues.mt");
        addTestFromFile("Function Parameter Float Conversion",
                        passPath + "functionParameterFloatConversion.mt");
        addTestFromFile("Method Parameter Type Checking",
                        passPath + "methodParameterTypeChecking.mt");
        addTestFromFile("Constructor Parameter Type Checking",
                        passPath + "constructorParameterTypeChecking.mt");
        addTestFromFile("Function Parameter With Namespaces",
                        passPath + "functionParameterWithNamespaces.mt");

        // Function return type tests
        addTestFromFile("Function Return Type Matching",
                        passPath + "functionReturnTypeMatching.mt");
        addTestFromFile("Namespace Return Types",
                        passPath + "namespaceReturnTypes.mt");
        addTestFromFile("Namespace Return Types Nested",
                        passPath + "namespaceReturnTypesNested.mt");
        addTestFromFile("Namespace Return Types With Class",
                        passPath + "namespaceReturnTypesWithClass.mt");
        addTestFromFile("Object Return Types",
                        passPath + "objectReturnTypes.mt");
        addTestFromFile("Object Return Types Function Returning Object",
                        passPath + "objectReturnTypesFunctionReturningObject.mt");
        addTestFromFile("Object Return Types Static Methods",
                        passPath + "objectReturnTypesStaticMethods.mt");
        addTestFromFile("Object Return Types Nested Objects",
                        passPath + "objectReturnTypesNestedObjects.mt");
        addTestFromFile("Object Return Types Namespace Returning Object",
                        passPath + "objectReturnTypesNamespaceReturningObject.mt");

        // String handling edge case tests
        addTestFromFile("String Concatenation with Null Objects",
                        passPath + "stringConcatenationWithNulls.mt");
        addTestFromFile("Empty String Edge Cases",
                        passPath + "emptyStringEdgeCases.mt");

        // Null propagation tests
        addTestFromFile("Null Propagation Complex Expressions",
                        passPath + "nullPropagationComplexExpressions.mt");

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
        addTestFromFile("Function Parameter With Namespaces Type Mismatch",
                        errorPath + "functionParameterWithNamespacesTypeMismatch.mt",
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
        addTestFromFile("Namespace Return Types Mismatch",
                        errorPath + "namespaceReturnTypesMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Namespace Return Types Wrong Class",
                        errorPath + "namespaceReturnTypesWrongClass.mt",
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
    }
}
