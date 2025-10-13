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
    }
}