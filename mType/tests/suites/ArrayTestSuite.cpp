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
    }
}