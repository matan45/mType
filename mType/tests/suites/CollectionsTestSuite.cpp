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
        addOutputVerificationTest("Array Operations",
                        passPath + "arrayOperations.mt");
        addOutputVerificationTest("Array Type Safety",
                        passPath + "arrayTypeSafety.mt");

        // List tests
        addOutputVerificationTest("Basic List Creation",
                        passPath + "listBasic.mt");
        addOutputVerificationTest("List Operations",
                        passPath + "listOperations.mt");

        // Map tests
        addOutputVerificationTest("Basic Map Creation",
                        passPath + "mapBasic.mt");
        addOutputVerificationTest("Map Literal Syntax",
                        passPath + "mapLiteral.mt");
        addOutputVerificationTest("Map Operations",
                        passPath + "mapOperations.mt");

        // Set tests
        addOutputVerificationTest("Basic Set Creation",
                        passPath + "setBasic.mt");
        addOutputVerificationTest("Set Operations",
                        passPath + "setOperations.mt");

        // Queue tests
        addOutputVerificationTest("Basic Queue Creation",
                        passPath + "queueBasic.mt");
        addOutputVerificationTest("Queue Operations",
                        passPath + "queueOperations.mt");

        // Stack tests
        addOutputVerificationTest("Basic Stack Creation",
                        passPath + "stackBasic.mt");
        addOutputVerificationTest("Stack Operations",
                        passPath + "stackOperations.mt");

        // For-each loop tests
        addOutputVerificationTest("For-Each with Array",
                        passPath + "forEachArray.mt");
        addOutputVerificationTest("For-Each with List",
                        passPath + "forEachList.mt");
        addOutputVerificationTest("For-Each with Map",
                        passPath + "forEachMap.mt");
        addOutputVerificationTest("For-Each with Set",
                        passPath + "forEachSet.mt");
        addOutputVerificationTest("For-Each Control Flow",
                        passPath + "forEachControlFlow.mt");

        // Integration tests
        addOutputVerificationTest("Collections as Parameters",
                        passPath + "collectionsAsParameters.mt");
        addOutputVerificationTest("Collections as Return Types",
                        passPath + "collectionsAsReturnTypes.mt");
        addOutputVerificationTest("Nested Collections",
                        passPath + "nestedCollections.mt");
        addOutputVerificationTest("Collections in Classes",
                        passPath + "collectionsInClasses.mt");

        // Error tests (expected to fail)
        addTestFromFile("Array Type Mismatch Error",
                        errorPath + "arrayTypeMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Map Key Type Error",
                        errorPath + "mapKeyTypeError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Index Access Error",
                        errorPath + "invalidIndexAccess.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("For-Each Type Mismatch Error",
                        errorPath + "forEachTypeMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Empty Collection Access Error",
                        errorPath + "emptyCollectionAccess.mt",
                        TestType::ERROR_EXPECTED);
    }
}