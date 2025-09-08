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
        addOutputVerificationTest("Advanced Array Operations",
                        passPath + "arrayAdvanced.mt");
        addOutputVerificationTest("Array with Objects",
                        passPath + "arrayObjects.mt");

        // Map tests
        addOutputVerificationTest("Basic Map Creation",
                        passPath + "mapBasic.mt");
        addOutputVerificationTest("Map Literal Syntax",
                        passPath + "mapLiteral.mt");
        addOutputVerificationTest("Advanced Map Operations",
                        passPath + "mapAdvanced.mt");
        addOutputVerificationTest("Map with Objects",
                        passPath + "mapObjects.mt");

        // Set tests
        addOutputVerificationTest("Basic Set Operations",
                        passPath + "setBasic.mt");
        addOutputVerificationTest("Set with Objects",
                        passPath + "setObjects.mt");
        addOutputVerificationTest("Set Iteration",
                        passPath + "setIteration.mt");
        addOutputVerificationTest("Two-Phase Comparison",
                        passPath + "twoPhaseComparison.mt");

        // Stack tests
        addOutputVerificationTest("Basic Stack Operations",
                        passPath + "stackBasic.mt");
        addOutputVerificationTest("Stack with Objects",
                        passPath + "stackObjects.mt");
        addOutputVerificationTest("Stack Iteration",
                        passPath + "stackIteration.mt");

        // Queue tests
        addOutputVerificationTest("Basic Queue Operations",
                        passPath + "queueBasic.mt");
        addOutputVerificationTest("Queue with Objects",
                        passPath + "queueObjects.mt");
        addOutputVerificationTest("Queue Iteration",
                        passPath + "queueIteration.mt");

        // For-each loop tests
        addOutputVerificationTest("For-Each with Array",
                        passPath + "forEachArray.mt");
        addOutputVerificationTest("Comprehensive Iteration",
                        passPath + "comprehensiveIteration.mt");

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
        addTestFromFile("Set Type Mismatch Error",
                        errorPath + "setTypeMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Stack Empty Pop Error",
                        errorPath + "stackEmptyPop.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Queue Empty Dequeue Error",
                        errorPath + "queueEmptyDequeue.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Map Key Not Found Error",
                        errorPath + "mapKeyNotFound.mt",
                        TestType::ERROR_EXPECTED);
        
    }
}