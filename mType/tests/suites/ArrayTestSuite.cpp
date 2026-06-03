#include "ArrayTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void ArrayTestSuite::setupTests()
    {
        // Add pass tests
        addOutputVerificationTest("Basic Array Declaration",
                                  passPath + "basicArrayDeclaration.mt");

        addOutputVerificationTest("Multi-dimensional Arrays",
                                  passPath + "multidimensionalArrays.mt");

        addOutputVerificationTest("Array as Parameter",
                                  passPath + "arrayAsParameter.mt");

        addOutputVerificationTest("Array as Data Member",
                                  passPath + "arrayAsDataMember.mt");

        addOutputVerificationTest("Generic Arrays",
                                  passPath + "genericArrays.mt");

        // Add new comprehensive tests
        addOutputVerificationTest("Array Pooling and Memory Management",
                                  passPath + "arrayPoolingAndMemory.mt");

        addOutputVerificationTest("Performance Characteristics",
                                  passPath + "performanceCharacteristics.mt");

        addOutputVerificationTest("Collection Interoperability",
                                  passPath + "collectionInteroperability.mt");

        addOutputVerificationTest("Multi-dimensional Edge Cases",
                                  passPath + "multiDimensionalEdgeCases.mt");

        addOutputVerificationTest("LinkedList Comprehensive",
                                  passPath + "linkedListComprehensive.mt");

        // Add array literal tests
        addOutputVerificationTest("Array Literals Basic",
                                  passPath + "arrayLiteralsBasic.mt");

        addOutputVerificationTest("Array Literals with Objects",
                                  passPath + "arrayLiteralsObjects.mt");

        addOutputVerificationTest("Array Literals Multi-dimensional",
                                  passPath + "arrayLiteralsMultiDimensional.mt");

        addOutputVerificationTest("Array Literals 4D",
                                  passPath + "arrayLiterals4D.mt");

        addOutputVerificationTest("N-Dimensional Arrays",
                                  passPath + "nDimensionalArrays.mt");

        addOutputVerificationTest("Compound Index Assignment",
                                  passPath + "compoundIndexAssignment.mt");

        // Phase 6: SIMD and SoA Optimization Tests
        addOutputVerificationTest("SIMD Primitive Arrays (int/float/bool) - 1D, 2D, 3D",
                                  passPath + "simdPrimitiveArrays.mt");

        addOutputVerificationTest("SIMD String Arrays (StringPool) - 1D, 2D, 3D",
                                  passPath + "simdStringArrays.mt");

        addOutputVerificationTest("SoA Object Arrays (Structure-of-Arrays) - 1D, 2D, 3D",
                                  passPath + "soaObjectArrays.mt");

        addOutputVerificationTest("Object Array Element Alias Write",
                                  passPath + "arrayElementAliasWrite.mt");

        addOutputVerificationTest("Generic Arrays with SIMD/SoA Optimization",
                                  passPath + "genericArraysOptimized.mt");

        addOutputVerificationTest("Comprehensive Optimization Verification",
                                  passPath + "optimizationVerification.mt");

        // Phase 7: SIMD-Accelerated Array Operations
        addOutputVerificationTest("SIMD Array Operations (Arithmetic, Reduction, Utility)",
                                  passPath + "arrayOperationsSIMD.mt");

        addOutputVerificationTest("Array Operations Comprehensive (Edge Cases, Chains, Mixed)",
                                  passPath + "arrayOperationsComprehensive.mt");

        // Phase 4: Box Type Arrays with SIMD/SoA Optimization
        addOutputVerificationTest("Int[] Box Arrays with SIMD",
                                  passPath + "intBoxSIMD.mt");

        addOutputVerificationTest("Int[] SoA Storage with SIMD (100 elements)",
                                  passPath + "intBoxSoASIMD.mt");

        addOutputVerificationTest("Float[] Box Arrays with SIMD",
                                  passPath + "floatBoxSIMD.mt");

        // === NULL & SPECIAL VALUES TESTS ===
        // Tests for null elements, special float values, boundary conditions, and defaults

        addOutputVerificationTest("Array Null Element Assignment",
                                  passPath + "arrayNullElementAssignment.mt");
        addOutputVerificationTest("Array Null in SIMD",
                                  passPath + "arrayNullInSIMD.mt");
        addOutputVerificationTest("Array Float Special Values",
                                  passPath + "arrayFloatSpecialValues.mt");
        addOutputVerificationTest("Array Integer Boundaries",
                                  passPath + "arrayIntegerBoundaries.mt");
        addOutputVerificationTest("Array Zero Length",
                                  passPath + "arrayZeroLength.mt");
        addOutputVerificationTest("Array Single Element",
                                  passPath + "arraySingleElement.mt");
        addOutputVerificationTest("Array Default Values",
                                  passPath + "arrayDefaultValues.mt");

        // === ARRAY OPERATIONS TESTS ===
        // Tests for array manipulation operations

        addOutputVerificationTest("Array Deep Copy",
                                  passPath + "arrayDeepCopy.mt");
        addOutputVerificationTest("Array Slicing",
                                  passPath + "arraySlicing.mt");
        addOutputVerificationTest("Array Concatenation",
                                  passPath + "arrayConcatenation.mt");
        addOutputVerificationTest("Array Equality",
                                  passPath + "arrayEquality.mt");
        addOutputVerificationTest("Array Linear Search",
                                  passPath + "arrayLinearSearch.mt");
        addOutputVerificationTest("Array Binary Search",
                                  passPath + "arrayBinarySearch.mt");
        addOutputVerificationTest("Array Sorting Custom",
                                  passPath + "arraySortingCustom.mt");
        addOutputVerificationTest("Array Map",
                                  passPath + "arrayMap.mt");
        addOutputVerificationTest("Array Filter",
                                  passPath + "arrayFilter.mt");
        addOutputVerificationTest("Array Reduce",
                                  passPath + "arrayReduce.mt");
        addOutputVerificationTest("Array Contains",
                                  passPath + "arrayContains.mt");
        addOutputVerificationTest("Array Last Index Of",
                                  passPath + "arrayLastIndexOf.mt");

        // === TYPE SYSTEM TESTS ===
        // Tests for type covariance, casting, and polymorphism

        addOutputVerificationTest("Array Covariance",
                                  passPath + "arrayCovariance.mt");
        addOutputVerificationTest("Array Type Casting",
                                  passPath + "arrayTypeCasting.mt");
        addOutputVerificationTest("Array Generic Variance",
                                  passPath + "arrayGenericVariance.mt");
        addOutputVerificationTest("Array Return Values",
                                  passPath + "arrayReturnValues.mt");
        addOutputVerificationTest("Array Return Inference",
                                  passPath + "arrayReturnInference.mt");
        addOutputVerificationTest("Array Interface Types",
                                  passPath + "arrayInterfaceTypes.mt");
        addOutputVerificationTest("Array Polymorphic Access",
                                  passPath + "arrayPolymorphicAccess.mt");

        // === MEMORY & LIFECYCLE TESTS ===
        // Tests for memory management, resizing, and cleanup

        addOutputVerificationTest("Array Dynamic Resize",
                                  passPath + "arrayDynamicResize.mt");
        addOutputVerificationTest("Array Memory Realloc",
                                  passPath + "arrayMemoryRealloc.mt");
        addOutputVerificationTest("Array Circular Ref",
                                  passPath + "arrayCircularRef.mt");
        addOutputVerificationTest("Array Garbage Collection",
                                  passPath + "arrayGarbageCollection.mt");
        addOutputVerificationTest("Array Copy On Write",
                                  passPath + "arrayCopyOnWrite.mt");
        addOutputVerificationTest("Array Memory Leak",
                                  passPath + "arrayMemoryLeak.mt");
        addOutputVerificationTest("Array Exception Cleanup",
                                  passPath + "arrayExceptionCleanup.mt");
        addOutputVerificationTest("Array Optimization Transition",
                                  passPath + "arrayOptimizationTransition.mt");

        // === ITERATION & ACCESS TESTS ===
        // Tests for iteration patterns and index access

        addOutputVerificationTest("Array Computed Indices",
                                  passPath + "arrayComputedIndices.mt");
        addOutputVerificationTest("Array Aliasing",
                                  passPath + "arrayAliasing.mt");
        addOutputVerificationTest("Array Multiple References",
                                  passPath + "arrayMultipleReferences.mt");
        addOutputVerificationTest("Array Nested Iteration",
                                  passPath + "arrayNestedIteration.mt");
        addOutputVerificationTest("Array Early Termination",
                                  passPath + "arrayEarlyTermination.mt");

        // === LANGUAGE INTEGRATION TESTS ===
        // Tests for integration with language features

        addOutputVerificationTest("Array In Closure",
                                  passPath + "arrayInClosure.mt");
        addOutputVerificationTest("Array Lambda Operations",
                                  passPath + "arrayLambdaOperations.mt");
        addOutputVerificationTest("Array Try Catch",
                                  passPath + "arrayTryCatch.mt");
        addOutputVerificationTest("Array Async Operations",
                                  passPath + "arrayAsyncOperations.mt");
        addOutputVerificationTest("Array Promise Array",
                                  passPath + "arrayPromiseArray.mt");
        addOutputVerificationTest("Array Imported",
                                  passPath + "arrayImported.mt");
        addOutputVerificationTest("Array Constant",
                                  passPath + "arrayConstant.mt");
        addOutputVerificationTest("Array In Finally",
                                  passPath + "arrayInFinally.mt");
        addOutputVerificationTest("Array Lambda Capture",
                                  passPath + "arrayLambdaCapture.mt");
        addOutputVerificationTest("Array Nested Lambda",
                                  passPath + "arrayNestedLambda.mt");

        // === EDGE CASES TESTS ===
        // Tests for edge cases and corner scenarios

        addOutputVerificationTest("Array Large Literal",
                                  passPath + "arrayLargeLiteral.mt");
        addOutputVerificationTest("Array Mixed Depth Literal",
                                  passPath + "arrayMixedDepthLiteral.mt");
        addOutputVerificationTest("Array Literal Side Effects",
                                  passPath + "arrayLiteralSideEffects.mt");
        addOutputVerificationTest("Array Jagged Mixed Dim",
                                  passPath + "arrayJaggedMixedDim.mt");
        addOutputVerificationTest("Array Ternary Operator",
                                  passPath + "arrayTernaryOperator.mt");
        addOutputVerificationTest("Array Default Parameter",
                                  passPath + "arrayDefaultParameter.mt");
        addOutputVerificationTest("Array Partial Init",
                                  passPath + "arrayPartialInit.mt");
        addOutputVerificationTest("Array Init Order",
                                  passPath + "arrayInitOrder.mt");

        // === PERFORMANCE & STRESS TESTS ===
        // Tests for performance characteristics and stress conditions

        addOutputVerificationTest("Array SIMD Threshold",
                                  passPath + "arraySIMDThreshold.mt");
        addOutputVerificationTest("Array Dense To Sparse",
                                  passPath + "arrayDenseToSparse.mt");
        addOutputVerificationTest("Array Cache Alignment",
                                  passPath + "arrayCacheAlignment.mt");
        addOutputVerificationTest("Array Tight Loop",
                                  passPath + "arrayTightLoop.mt");
        addOutputVerificationTest("Array Stress",
                                  passPath + "arrayStress.mt");

        // === CROSS-FEATURE EDGE CASES ===
        addOutputVerificationTest("Value Class Array For-Each",
                                  passPath + "valueClassArrayForEach.mt");

        // Add error tests
        addTestFromFile("Negative Array Size",
                        errorPath + "negativeArraySize.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Out of Bounds Access",
                        errorPath + "outOfBoundsAccess.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Type Mismatch",
                        errorPath + "typeMismatch.mt",
                        TestType::ERROR_EXPECTED);

        // Add new error tests
        addTestFromFile("Large Array Out of Bounds",
                        errorPath + "largeArrayOutOfBounds.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Sparse Array Bounds",
                        errorPath + "sparseArrayBounds.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Multi-dimensional Bounds",
                        errorPath + "multiDimensionalBounds.mt",
                        TestType::ERROR_EXPECTED);

        // Add array literal error tests
        addTestFromFile("Array Literal Type Mismatch",
                        errorPath + "arrayLiteralTypeMismatch.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Array Literal Mixed Types",
                        errorPath + "arrayLiteralMixedTypes.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Array Literal Object Type Mismatch",
                        errorPath + "arrayLiteralObjectTypeMismatch.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Array Literal Null Mixed",
                        errorPath + "arrayLiteralNullMixed.mt",
                        TestType::ERROR_EXPECTED);

        // === NULL & SPECIAL VALUES ERROR TESTS ===
        addTestFromFile("Array Uninitialized Access",
                        errorPath + "arrayUninitializedAccess.mt",
                        TestType::ERROR_EXPECTED);

        // === TYPE SYSTEM ERROR TESTS ===
        addTestFromFile("Array Dimension Type Check",
                        errorPath + "arrayDimensionTypeCheck.mt",
                        TestType::ERROR_EXPECTED);

        // === ITERATION & ACCESS ERROR TESTS ===
        addTestFromFile("Array Modify During Iteration",
                        errorPath + "arrayModifyDuringIteration.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Array Iterator Invalidation",
                        errorPath + "arrayIteratorInvalidation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Array Off By One",
                        errorPath + "arrayOffByOne.mt",
                        TestType::ERROR_EXPECTED);

        // === EDGE CASES ERROR TESTS ===
        addTestFromFile("Array Literal Inference",
                        errorPath + "arrayLiteralInference.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Array Empty Literal Ambiguous",
                        errorPath + "arrayEmptyLiteralAmbiguous.mt",
                        TestType::ERROR_EXPECTED);

        // === PERFORMANCE & STRESS ERROR TESTS ===
        addTestFromFile("Array Large Allocation",
                        errorPath + "arrayLargeAllocation.mt",
                        TestType::ERROR_EXPECTED);

        // === EDGE CASE TESTS - empty length+iter / array of lambdas ===
        addOutputVerificationTest("Empty Array Length And Iteration",
                        passPath + "emptyArrayLengthAndIteration.mt");
        addOutputVerificationTest("Array Of Lambdas",
                        passPath + "arrayOfLambdas.mt");

        // === MYT-281: arrays as Object subtypes ===
        // Pass tests — Object widening, isClassOf Object, (T[])obj round-trip,
        // List<Object> storage, multi-dim Object widening, primitive
        // element preservation through Object slots. The Object-method
        // dispatch (arr.toString()/equals/hashCode) is deferred to a
        // follow-up that lands the type-checker side; that single test is
        // not in this set.
        addOutputVerificationTest("Array Assign To Object",
                        passPath + "objectSubtype/arrayAssignToObject.mt");
        addOutputVerificationTest("Array Assign To Object Field",
                        passPath + "objectSubtype/arrayAssignToObjectField.mt");
        addOutputVerificationTest("Array IsClassOf Object",
                        passPath + "objectSubtype/arrayIsClassOfObject.mt");
        addOutputVerificationTest("Array IsClassOf Self",
                        passPath + "objectSubtype/arrayIsClassOfSelf.mt");
        addOutputVerificationTest("Array Downcast From Object",
                        passPath + "objectSubtype/arrayDowncastFromObject.mt");
        addOutputVerificationTest("Array Length Still Works",
                        passPath + "objectSubtype/arrayLengthStillWorks.mt");
        addOutputVerificationTest("Array Object Identity",
                        passPath + "objectSubtype/arrayObjectIdentity.mt");
        addOutputVerificationTest("Array Boxing Primitive Element",
                        passPath + "objectSubtype/arrayBoxingPrimitiveElement.mt");
        addOutputVerificationTest("Multidim Array As Object",
                        passPath + "objectSubtype/multidimArrayAsObject.mt");
        addOutputVerificationTest("Array In List Of Object",
                        passPath + "objectSubtype/arrayInListOfObject.mt");
        // MYT-282: deferred tests now registered.
        addOutputVerificationTest("Array In Generic Object Param",
                        passPath + "objectSubtype/arrayInGenericObjectParam.mt");
        addOutputVerificationTest("Array Object Methods Callable",
                        passPath + "objectSubtype/arrayObjectMethodsCallable.mt");

        // MYT-281/282 edge cases — Object-method dispatch across element
        // types, cross-feature widening (returns / fields / generics),
        // mixed Object[] storage, equality / hashCode semantics,
        // isClassOf negation, multi-stage Object pass-through.
        addOutputVerificationTest("Array Object Methods Across Types",
                        passPath + "objectSubtype/arrayObjectMethodsAcrossTypes.mt");
        addOutputVerificationTest("Array Object Methods Object Element",
                        passPath + "objectSubtype/arrayObjectMethodsObjectElement.mt");
        addOutputVerificationTest("Array Returned As Object",
                        passPath + "objectSubtype/arrayReturnedAsObject.mt");
        addOutputVerificationTest("Array In Class Field List",
                        passPath + "objectSubtype/arrayInClassFieldList.mt");
        addOutputVerificationTest("Mixed Object Array With Arrays And Instances",
                        passPath + "objectSubtype/mixedObjectArrayWithArraysAndInstances.mt");
        addOutputVerificationTest("Array Object Equals Reference Semantics",
                        passPath + "objectSubtype/arrayObjectEqualsReferenceSemantics.mt");
        addOutputVerificationTest("Array Object IsClassOf Negation",
                        passPath + "objectSubtype/arrayObjectIsClassOfNegation.mt");
        addOutputVerificationTest("Array Passed As Object Arg",
                        passPath + "objectSubtype/arrayPassedAsObjectArg.mt");
        addOutputVerificationTest("Array HashCode Stable",
                        passPath + "objectSubtype/arrayHashCodeStable.mt");

        // === MYT-378: polymorphic object arrays (SoA -> heterogeneous fallback) ===
        // Object arrays of size >= 16 use SoA storage that can only faithfully
        // hold the exact declared class. Storing a subtype (or anything into
        // Object[]) now falls back to heterogeneous Value storage instead of
        // throwing "ObjectInstance class mismatch", preserving subtype fields
        // and polymorphic identity. The homogeneous case stays on the fast path.
        addOutputVerificationTest("Array Holds Subtypes Size16",
                        passPath + "objectSubtype/objectArrayHoldsSubtypes_size16.mt");
        addOutputVerificationTest("Object Slot Array Size16",
                        passPath + "objectSubtype/objectSlotArray_size16.mt");
        addOutputVerificationTest("Object Array Homogeneous Fast Path",
                        passPath + "objectSubtype/objectArrayHomogeneousFastPath.mt");
        addOutputVerificationTest("Multidim Object Array Subtypes",
                        passPath + "objectSubtype/multidimObjectArraySubtypes.mt");

        // --- MYT-378 small arrays (< 16, non-SoA Value[] path) ---
        // The ticket's literal repro is small (`new Animal[1]`, `Object[4]`).
        // These never touch SoA; they pin the plain Value[] store path.
        addOutputVerificationTest("Array Holds Subtypes Small",
                        passPath + "objectSubtype/objectArrayHoldsSubtypes_small.mt");
        addOutputVerificationTest("Object Slot Array Small",
                        passPath + "objectSubtype/objectSlotArray_small.mt");

        // --- MYT-378 SoA -> heterogeneous conversion mid-stream ---
        // Populate the SoA fast path with the exact declared class first, then
        // store a subtype: convertToHeterogeneous() must migrate the existing
        // columns intact. (The size16 fixture above stores a subtype at index
        // 0, so SoA is bypassed; this exercises the populated-then-demoted path.)
        addOutputVerificationTest("Array SoA Then Subtype Size16",
                        passPath + "objectSubtype/objectArraySoaThenSubtype_size16.mt");

        // --- MYT-378 deep inheritance + element overwrite ---
        addOutputVerificationTest("Array Deep Inheritance Size16",
                        passPath + "objectSubtype/objectArrayDeepInheritance_size16.mt");
        addOutputVerificationTest("Array Overwrite Element Size16",
                        passPath + "objectSubtype/objectArrayOverwriteElement_size16.mt");

        // --- MYT-378 interface / null / value-class at SoA size ---
        // Each is non-exact vs the declared element class, so each rides the
        // heterogeneous fallback at size 16.
        addOutputVerificationTest("Interface Array Holds Implementors Size16",
                        passPath + "objectSubtype/interfaceArrayHoldsImplementors_size16.mt");
        addOutputVerificationTest("Array Null Interleaved Size16",
                        passPath + "objectSubtype/objectArrayNullInterleaved_size16.mt");
        addOutputVerificationTest("Object Slot Array Value Class Size16",
                        passPath + "objectSubtype/objectSlotArrayValueClass_size16.mt");

        // --- MYT-378 soundness: wrong-sibling cast after widening must throw ---
        // Widening a Dog into Object[]/Animal[] is allowed, but the fallback
        // must preserve the element's runtime class so narrowing to a sibling
        // (Cat) still fails. Substring pins the rendered cast message.
        addTestFromFile("Object Element Cast Wrong Sibling",
                        errorPath + "objectSubtype/objectElementCastWrongSibling_error.mt",
                        TestType::ERROR_EXPECTED,
                        "Cannot cast Dog to Cat");
        addTestFromFile("Subtype Element Cast Wrong Sibling Size16",
                        errorPath + "objectSubtype/subtypeElementCastWrongSibling_size16_error.mt",
                        TestType::ERROR_EXPECTED,
                        "Cannot cast Dog to Cat");

        // Edge-case error tests — wrong-target casts after Object widening.
        addTestFromFile("Array Cast To Concrete Class",
                        errorPath + "objectSubtype/arrayCastToConcreteClass_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Array Cast To Wrong Dimensions",
                        errorPath + "objectSubtype/arrayCastToWrongDimensions_error.mt",
                        TestType::ERROR_EXPECTED);
        // MYT-281: multi-dim cast soundness — rank and element type are
        // both validated against the runtime bridge. Substring pins the
        // rendered message ("cannot cast <runtime> to <target>") so a
        // future regression silently producing a different error type
        // surfaces here.
        addTestFromFile("Multidim Array Cast Rank Too High",
                        errorPath + "objectSubtype/multidimArrayCastRankTooHigh_error.mt",
                        TestType::ERROR_EXPECTED,
                        "cannot cast int[][][] to int[][]");
        addTestFromFile("Multidim Array Cast Rank Too Low",
                        errorPath + "objectSubtype/multidimArrayCastRankTooLow_error.mt",
                        TestType::ERROR_EXPECTED,
                        "cannot cast int[][] to int[][][]");
        addTestFromFile("Multidim Array Cast Element Mismatch",
                        errorPath + "objectSubtype/multidimArrayCastElementMismatch_error.mt",
                        TestType::ERROR_EXPECTED,
                        "cannot cast int[][] to float[][]");
        addTestFromFile("Non-Array Object Cast To Array",
                        errorPath + "objectSubtype/nonArrayObjectCastToArray_error.mt",
                        TestType::ERROR_EXPECTED);
        // MYT-137: strict declaration-time array invariance. `Animal[] a = new
        // Dog[1]` must error at compile time — the array-store soundness hole
        // is closed by rejecting the alias before it can form.
        addTestFromFile("Array Decl Covariance Strict",
                        errorPath + "objectSubtype/arrayDeclCovarianceStrict_error.mt",
                        TestType::ERROR_EXPECTED,
                        "Array covariance violation");

        // Error tests — store-time invariance pins the soundness boundary
        // and wrong-element casts must throw at runtime. The MYT-279
        // metadata regression for primitive-element store wrong-type is
        // already covered by the existing arrayDimensionTypeCheck.mt test
        // (int[][] slot rejecting string[]); a top-level
        // `int[] a; a[0] = "x"` does NOT error today (NativeArray
        // setUnchecked falls back to heterogeneous storage), so no test is
        // pinned here for that surface.
        addTestFromFile("Array Store Invariance",
                        errorPath + "objectSubtype/arrayStoreInvariance_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Array Downcast Wrong Element",
                        errorPath + "objectSubtype/arrayDowncastWrongElement_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Array Alias Invariance",
                        errorPath + "objectSubtype/arrayAliasInvariance_error.mt",
                        TestType::ERROR_EXPECTED);
        // arrayDeclCovarianceStrict_error.mt is intentionally left
        // unregistered: it pins MYT-137 strict-invariance, which is blocked
        // on array-literal target-type inference. Flip on with MYT-137.
    }
}
