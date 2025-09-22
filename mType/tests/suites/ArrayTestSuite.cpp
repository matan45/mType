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
    }
}