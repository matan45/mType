#include "EnhancedForLoopTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void EnhancedForLoopTestSuite::setupTests()
    {
        // Add pass tests for enhanced for-loop syntax: for(T x : collection)
        addOutputVerificationTest("Enhanced For-Loop with ArrayList",
                                  passPath + "forEachArrayList.mt");

        addOutputVerificationTest("Enhanced For-Loop with HashSet",
                                  passPath + "forEachHashSet.mt");

        addOutputVerificationTest("Enhanced For-Loop with HashMap",
                                  passPath + "forEachHashMap.mt");

        addOutputVerificationTest("Enhanced For-Loop with LinkedList",
                                  passPath + "forEachLinkedList.mt");

        addOutputVerificationTest("Enhanced For-Loop with Queue",
                                  passPath + "forEachQueue.mt");

        addOutputVerificationTest("Enhanced For-Loop with Stack",
                                  passPath + "forEachStack.mt");

        addOutputVerificationTest("Nested Enhanced For-Loops",
                                  passPath + "forEachNested.mt");

        addOutputVerificationTest("Enhanced For-Loop with Empty Collection",
                                  passPath + "forEachEmpty.mt");

        // Add error tests for iterator type safety
        addTestFromFile("For-Each on Non-Iterable Type",
                       errorPath + "forEachNonIterable.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("For-Each with Type Mismatch",
                       errorPath + "forEachTypeMismatch.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("For-Each with Primitive Type Mismatch",
                       errorPath + "forEachPrimitiveMismatch.mt",
                       TestType::ERROR_EXPECTED);
    }
}
