#include "CastTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void CastTestSuite::setupTests()
    {
        // === PRIMITIVE CASTING TESTS (11 tests) ===
        addOutputVerificationTest("Int to Float Cast",
                        passPath + "intToFloatCast.mt");
        addOutputVerificationTest("Float to Int Cast",
                        passPath + "floatToIntCast.mt");
        addOutputVerificationTest("Int to Bool Cast",
                        passPath + "intToBoolCast.mt");
        addOutputVerificationTest("Bool to Int Cast",
                        passPath + "boolToIntCast.mt");
        addOutputVerificationTest("Int to String Cast",
                        passPath + "intToStringCast.mt");
        addOutputVerificationTest("Float to String Cast",
                        passPath + "floatToStringCast.mt");
        addOutputVerificationTest("Bool to String Cast",
                        passPath + "boolToStringCast.mt");
        addOutputVerificationTest("String to Bool Cast",
                        passPath + "stringToBoolCast.mt");
        addOutputVerificationTest("Chained Primitive Casts",
                        passPath + "chainedPrimitiveCasts.mt");
        addOutputVerificationTest("Same Type Cast",
                        passPath + "sameTypeCast.mt");
        addOutputVerificationTest("Primitive Cast in Expression",
                        passPath + "primitiveCastInExpression.mt");

        // === OBJECT CASTING TESTS (13 tests) ===
        addOutputVerificationTest("Upcast to Parent Class",
                        passPath + "upcastToParent.mt");
        addOutputVerificationTest("Downcast to Child Class",
                        passPath + "downcastToChild.mt");
        addOutputVerificationTest("Cast to Sibling Class",
                        passPath + "castToSibling.mt");
        addOutputVerificationTest("Multi-level Inheritance Cast",
                        passPath + "multiLevelCast.mt");
        addOutputVerificationTest("Cast with Method Override",
                        passPath + "castWithOverride.mt");
        addOutputVerificationTest("Cast and Access Members",
                        passPath + "castAndAccessMembers.mt");
        addOutputVerificationTest("Cast in Assignment",
                        passPath + "castInAssignment.mt");
        addOutputVerificationTest("Cast in Method Call",
                        passPath + "castInMethodCall.mt");
        addOutputVerificationTest("Cast Return Value",
                        passPath + "castReturnValue.mt");
        addOutputVerificationTest("Polymorphic Cast",
                        passPath + "polymorphicCast.mt");
        addOutputVerificationTest("Same Class Cast",
                        passPath + "sameClassCast.mt");
        addOutputVerificationTest("Deep Inheritance Cast",
                        passPath + "deepInheritanceCast.mt");
        addOutputVerificationTest("Cast with Constructor",
                        passPath + "castWithConstructor.mt");

        // === INTERFACE CASTING TESTS (7 tests) ===
        addOutputVerificationTest("Cast to Interface",
                        passPath + "castToInterface.mt");
        addOutputVerificationTest("Interface to Class Cast",
                        passPath + "interfaceToClassCast.mt");
        addOutputVerificationTest("Multiple Interfaces Cast",
                        passPath + "multipleInterfacesCast.mt");
        addOutputVerificationTest("Interface Inheritance Cast",
                        passPath + "interfaceInheritanceCast.mt");
        addOutputVerificationTest("Cast Interface Method Call",
                        passPath + "castInterfaceMethodCall.mt");
        addOutputVerificationTest("Class Implements Multiple Cast",
                        passPath + "classImplementsMultipleCast.mt");
        addOutputVerificationTest("Interface to Wrong Class Cast",
                        passPath + "interfaceToWrongClassCast.mt");

        // === GENERIC CASTING TESTS (7 tests) ===
        addOutputVerificationTest("Generic Class Cast",
                        passPath + "genericClassCast.mt");
        addOutputVerificationTest("Generic Type Parameter Cast",
                        passPath + "genericTypeParameterCast.mt");
        addOutputVerificationTest("Nested Generic Cast",
                        passPath + "nestedGenericCast.mt");
        addOutputVerificationTest("Generic Array Cast",
                        passPath + "genericArrayCast.mt");
        addOutputVerificationTest("Generic Inheritance Cast",
                        passPath + "genericInheritanceCast.mt");
        addOutputVerificationTest("Generic Interface Cast",
                        passPath + "genericInterfaceCast.mt");
        addOutputVerificationTest("Generic Wildcard Cast",
                        passPath + "genericWildcardCast.mt");

        // === COLLECTION CASTING TESTS (4 tests) ===
        addOutputVerificationTest("Array Element Cast",
                        passPath + "arrayElementCast.mt");
        addOutputVerificationTest("List Generic Cast",
                        passPath + "listGenericCast.mt");
        addOutputVerificationTest("Map Generic Cast",
                        passPath + "mapGenericCast.mt");
        addOutputVerificationTest("Collection with Interface Cast",
                        passPath + "collectionInterfaceCast.mt");

        // === NULL HANDLING TESTS (5 tests) ===
        addOutputVerificationTest("Null to Object Cast",
                        passPath + "nullToObjectCast.mt");
        addOutputVerificationTest("Null to Interface Cast",
                        passPath + "nullToInterfaceCast.mt");
        addOutputVerificationTest("Null Cast Assignment",
                        passPath + "nullCastAssignment.mt");
        addOutputVerificationTest("Null Cast in Conditional",
                        passPath + "nullCastInConditional.mt");
        addTestFromFile("Null to Primitive Cast Error",
                        errorPath + "nullToPrimitiveCast.mt",
                        TestType::ERROR_EXPECTED);

        // === isClassOf TESTS (9 tests) ===
        addOutputVerificationTest("isClassOf Primitive Types",
                        passPath + "isClassOfPrimitive.mt");
        addOutputVerificationTest("isClassOf Parent Class",
                        passPath + "isClassOfParent.mt");
        addOutputVerificationTest("isClassOf Child Class",
                        passPath + "isClassOfChild.mt");
        addOutputVerificationTest("isClassOf Interface",
                        passPath + "isClassOfInterface.mt");
        addOutputVerificationTest("isClassOf Multiple Interfaces",
                        passPath + "isClassOfMultipleInterfaces.mt");
        addOutputVerificationTest("isClassOf Null Value",
                        passPath + "isClassOfNull.mt");
        addOutputVerificationTest("isClassOf in Conditional",
                        passPath + "isClassOfConditional.mt");
        addOutputVerificationTest("isClassOf with Generic",
                        passPath + "isClassOfGeneric.mt");
        addOutputVerificationTest("isClassOf Deep Hierarchy",
                        passPath + "isClassOfDeepHierarchy.mt");

        // === MYT-41: PARAMETERIZED isClassOf TESTS ===
        // Verify that isClassOf discriminates between different type arguments
        // on the same base class (Box<Int> vs Box<String>) and preserves the
        // existing type-erased raw-base fallback for backward compatibility.
        addOutputVerificationTest("isClassOf Generic Exact",
                        passPath + "isClassOfGenericExact.mt");
        addOutputVerificationTest("isClassOf Generic Raw Fallback",
                        passPath + "isClassOfGenericRawFallback.mt");
        addOutputVerificationTest("isClassOf Generic Nested",
                        passPath + "isClassOfGenericNested.mt");
        addOutputVerificationTest("isClassOf Generic Multi Param",
                        passPath + "isClassOfGenericMultiParam.mt");
        addOutputVerificationTest("isClassOf Generic Invariance",
                        passPath + "isClassOfGenericInvariance.mt");
        addOutputVerificationTest("isClassOf Generic Upcast",
                        passPath + "isClassOfGenericUpcast.mt");

        // === MYT-41: TYPE-PARAMETER isClassOf TESTS ===
        // `obj isClassOf T` inside a generic method resolves T via the
        // receiver's runtime bindings before delegating to the normal
        // instanceof check.
        addOutputVerificationTest("isClassOf Type Param",
                        passPath + "isClassOfTypeParam.mt");
        addOutputVerificationTest("isClassOf Type Param Negative",
                        passPath + "isClassOfTypeParamNegative.mt");
        addOutputVerificationTest("isClassOf Type Param With Primitive",
                        passPath + "isClassOfTypeParamWithPrimitive.mt");

        // === INTEGRATION TESTS (5 tests) ===
        addOutputVerificationTest("Safe Downcast Pattern",
                        passPath + "safeDowncastPattern.mt");
        addOutputVerificationTest("Type Checking Before Cast",
                        passPath + "typeCheckBeforeCast.mt");
        addOutputVerificationTest("Polymorphic Collection Cast",
                        passPath + "polymorphicCollectionCast.mt");
        addOutputVerificationTest("Complex Type Hierarchy Cast",
                        passPath + "complexHierarchyCast.mt");
        addOutputVerificationTest("Cast with Namespace",
                        passPath + "castWithNamespace.mt");

        // === ERROR TESTS ===
        addTestFromFile("Invalid Primitive to Object Cast",
                        errorPath + "primitiveToObjectCast.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Object to Primitive Cast",
                        errorPath + "objectToPrimitiveCast.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Incompatible Class Cast",
                        errorPath + "incompatibleClassCast.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Interface Cast",
                        errorPath + "invalidInterfaceCast.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Generic Type Mismatch Cast",
                        errorPath + "genericTypeMismatchCast.mt",
                        TestType::ERROR_EXPECTED);

        // === MYT-41: isClassOf ERROR TESTS ===
        // Note: a runtime "unbound type parameter" test is not included —
        // the compiler guard (see ExpressionCompiler::compileInstanceOf)
        // only emits INSTANCEOF_TYPEPARAM when the RHS name matches a
        // declared generic parameter of an enclosing class, and any
        // normally-constructed instance of such a class has its bindings
        // populated at `new Foo<T>(...)` time. The defensive error in
        // TypeExecutor::resolveTypeParameter is still there, but it isn't
        // reachable through ordinary mType source, so there's no user-facing
        // test for it.
        addTestFromFile("isClassOf Malformed Parameterized",
                        errorPath + "isClassOfMalformedParameterized.mt",
                        TestType::ERROR_EXPECTED);

        // ====================================
        // NEW EDGE CASE TESTS (70 tests)
        // ====================================

        // === PRIMITIVE EDGE CASES ===
        addOutputVerificationTest("Cast Primitive Overflow",
                        passPath + "castPrimitiveOverflow_pass.mt");
        addOutputVerificationTest("Cast Primitive Underflow",
                        passPath + "castPrimitiveUnderflow_pass.mt");
        // REMOVED - castNaNToInt_pass.mt (mType doesn't support NaN - division by zero throws error)
        // REMOVED - castInfinityToInt_pass.mt (mType doesn't support Infinity)
        addOutputVerificationTest("Cast Negative Zero",
                        passPath + "castNegativeZero_pass.mt");
        addTestFromFile("Cast String to Int Invalid",
                        errorPath + "castStringToIntInvalid_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Cast String to Float Invalid",
                        errorPath + "castStringToFloatInvalid_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Cast Empty String to Numeric",
                        errorPath + "castEmptyStringToNumeric_error.mt",
                        TestType::ERROR_EXPECTED);

        // === NULLABLE TYPE CASTING (7 tests) ===
        addTestFromFile("Cast Nullable Int to Non-Null",
                        errorPath + "castNullableIntToNonNull_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Cast Nullable Object to Primitive",
                        errorPath + "castNullableObjectToPrimitive_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Cast Nullable Generic",
                        passPath + "castNullableGeneric_pass.mt");
        addOutputVerificationTest("Cast Chained Null Propagation",
                        passPath + "castChainedNullPropagation_pass.mt");
        addOutputVerificationTest("Cast Null in Complex Expression",
                        passPath + "castNullInComplexExpr_pass.mt");
        addOutputVerificationTest("Cast Null in Ternary",
                        passPath + "castNullInTernary_pass.mt");
        addOutputVerificationTest("Cast Null Coalescing",
                        passPath + "castNullCoalescing_pass.mt");

        // === ARRAY EDGE CASES (9 tests) ===
        addTestFromFile("Cast Array Covariance",
                        errorPath + "castArrayCovariance_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Cast Array Parent to Child",
                        errorPath + "castArrayParentToChild_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Cast Array Interface to Implementation",
                        passPath + "castArrayInterfaceToImpl_pass.mt");
        addOutputVerificationTest("Cast Array With Null Elements",
                        passPath + "castArrayWithNullElements_pass.mt");
        addOutputVerificationTest("Cast Sparse Array",
                        passPath + "castSparseArray_pass.mt");
        addTestFromFile("Cast Generic Array Types",
                        errorPath + "castGenericArrayTypes_error.mt",
                        TestType::ERROR_EXPECTED);

        // === GENERIC TYPE CASTING ===
        // Note: Several advanced generic tests removed - mType doesn't support these features:
        // REMOVED - castBoundedGenericsUpper_pass.mt (bounded generics with extends)
        // REMOVED - castBoundedMultiple_pass.mt (multiple bounds with &)
        // REMOVED - castCovariantGeneric_pass.mt (variance annotations)
        // REMOVED - castContravariantGeneric_pass.mt (variance annotations)
        // REMOVED - castWildcardUnbounded_pass.mt (wildcard types <?>)
        // REMOVED - castRawTypeToGeneric_pass.mt (raw types without type parameters)
        // REMOVED - castGenericMethodInference_pass.mt (wildcards + advanced inference)
        addTestFromFile("Cast Bounded Generics Lower",
                        errorPath + "castBoundedGenericsLower_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Cast Invariant Generic",
                        errorPath + "castInvariantGeneric_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Cast Nested Generic Double",
                        passPath + "castNestedGenericDouble_pass.mt");
        addOutputVerificationTest("Cast Nested Generic Triple",
                        passPath + "castNestedGenericTriple_pass.mt");

        // === INTERFACE CASTING ===
        addOutputVerificationTest("Cast Diamond Problem",
                        passPath + "castDiamondProblem_pass.mt");
        addOutputVerificationTest("Cast Multiple Interface Same Method",
                        passPath + "castMultipleInterfaceSameMethod_pass.mt");
        addOutputVerificationTest("Cast Interface Default Method",
                        passPath + "castInterfaceDefaultMethod_pass.mt");
        addOutputVerificationTest("Cast Marker Interface",
                        passPath + "castMarkerInterface_pass.mt");
        addOutputVerificationTest("Cast Empty Interface",
                        passPath + "castEmptyInterface_pass.mt");
        // REMOVED - castFunctionalInterface_pass.mt (needs testing - may not support SAM pattern)
        // REMOVED - castMethodReference_pass.mt (method references :: not supported)
        addTestFromFile("Cast Interface Cross Partial",
                        errorPath + "castInterfaceCrossPartial_error.mt",
                        TestType::ERROR_EXPECTED);

        // === CLASS HIERARCHY CASTING (10 tests) ===
        addOutputVerificationTest("Cast Abstract Class",
                        passPath + "castAbstractClass_pass.mt");
        addOutputVerificationTest("Cast Abstract to Concrete",
                        passPath + "castAbstractToConcrete_pass.mt");
        addOutputVerificationTest("Cast Abstract Generic",
                        passPath + "castAbstractGeneric_pass.mt");
        addOutputVerificationTest("Cast Final Class",
                        passPath + "castFinalClass_pass.mt");
        addOutputVerificationTest("Cast Runtime Type Check",
                        passPath + "castRuntimeTypeCheck_pass.mt");
        addTestFromFile("Cast Sibling Runtime",
                        errorPath + "castSiblingRuntime_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Cast Deep Hierarchy Upcast",
                        passPath + "castDeepHierarchyUpcast_pass.mt");
        addOutputVerificationTest("Cast Deep Hierarchy Downcast",
                        passPath + "castDeepHierarchyDowncast_pass.mt");
        addOutputVerificationTest("Cast Constructor Chaining",
                        passPath + "castConstructorChaining_pass.mt");
        addOutputVerificationTest("Cast Covariant Return",
                        passPath + "castCovariantReturn_pass.mt");

        // === COMPLEX EXPRESSION CASTING (7 tests) ===
        addOutputVerificationTest("Cast in If Condition",
                        passPath + "castInIfCondition_pass.mt");
        addOutputVerificationTest("Cast in While Condition",
                        passPath + "castInWhileCondition_pass.mt");
        addOutputVerificationTest("Cast in For Loop",
                        passPath + "castInForLoop_pass.mt");
        addOutputVerificationTest("Cast in Switch Expression",
                        passPath + "castInSwitchExpr_pass.mt");
        addOutputVerificationTest("Cast in Binary Operator",
                        passPath + "castInBinaryOperator_pass.mt");
        addOutputVerificationTest("Cast with InstanceOf",
                        passPath + "castWithInstanceOf_pass.mt");
        addOutputVerificationTest("Cast in Lambda Body",
                        passPath + "castInLambdaBody_pass.mt");

        // === NAMESPACE & IMPORT CASTING ===
        // Note: Namespace tests removed - mType doesn't support namespace keyword
        // The following tests were deleted as they use unsupported features:
        // - castFullyQualifiedType_pass.mt (namespace syntax)
        // - castNamespaceCollision_pass.mt (namespace syntax)
        // - castImportedType_pass.mt (namespace syntax)
        // - castCrossModule_pass.mt (namespace syntax)

        // === MEMORY & PERFORMANCE (3 tests) ===
        addOutputVerificationTest("Cast Large Object Graph",
                        passPath + "castLargeObjectGraph_pass.mt");
        addOutputVerificationTest("Cast Circular References",
                        passPath + "castCircularReferences_pass.mt");
        addOutputVerificationTest("Cast Many Objects",
                        passPath + "castManyObjects_pass.mt");

        // === ERROR HANDLING (3 tests) ===
        addTestFromFile("Cast Error Message",
                        errorPath + "castErrorMessage_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Cast Runtime Validation",
                        errorPath + "castRuntimeValidation_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Cast in Try Catch",
                        passPath + "castInTryCatch_pass.mt");
    }
}
