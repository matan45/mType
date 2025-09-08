#include "CollectionsTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void CollectionsTestSuite::setupTests()
    {
        // Array tests
        addOutputVerificationTest("Basic Array Creation",
                        passPath + "arrayBasic.mt");
        addOutputVerificationTest("Array Literal Syntax",
                        passPath + "arrayLiteral.mt");

        // Map tests
        addOutputVerificationTest("Basic Map Creation",
                        passPath + "mapBasic.mt");
        addOutputVerificationTest("Map Literal Syntax",
                        passPath + "mapLiteral.mt");

        // For-each loop tests
        addOutputVerificationTest("For-Each with Array",
                        passPath + "forEachArray.mt");

        // Integration tests
        addOutputVerificationTest("Collections as Parameters",
                        passPath + "collectionsAsParameters.mt");
        addOutputVerificationTest("Collections as Return Types",
                        passPath + "collectionsAsReturnTypes.mt");
        addOutputVerificationTest("Nested Collections",
                        passPath + "nestedCollections.mt");
        addOutputVerificationTest("Comprehensive Nested For-Each",
                        passPath + "comprehensiveNestedForEach.mt");

        // Error tests (expected to fail)
        addTestFromFile("Array Type Mismatch Error",
                        errorPath + "arrayTypeMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("For-Each Type Mismatch Error",
                        errorPath + "forEachTypeMismatch.mt",
                        TestType::ERROR_EXPECTED);
    }
}