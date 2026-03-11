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
    }
}