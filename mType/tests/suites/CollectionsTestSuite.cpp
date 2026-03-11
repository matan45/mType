#include "CollectionsTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void CollectionsTestSuite::setupTests()
    {
        // ArrayList sorting tests
        addOutputVerificationTest("ArrayList Sort Ascending",
                                  passPath + "arrayListSort.mt");

        addOutputVerificationTest("ArrayList Sort Descending",
                                  passPath + "arrayListSortDescending.mt");

        addOutputVerificationTest("ArrayList Sort Strings by Length",
                                  passPath + "arrayListSortStrings.mt");

        // LinkedList sorting tests
        addOutputVerificationTest("LinkedList Sort Ascending",
                                  passPath + "linkedListSort.mt");

        addOutputVerificationTest("LinkedList Sort Descending",
                                  passPath + "linkedListSortDescending.mt");

        // === ERROR TESTS ===
        addTestFromFile("ArrayList Get Out of Bounds",
                        errorPath + "arrayListGetOutOfBounds.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("ArrayList Set Out of Bounds",
                        errorPath + "arrayListSetOutOfBounds.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("ArrayList Get Negative Index",
                        errorPath + "arrayListGetNegativeIndex.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("ArrayList Set Negative Index",
                        errorPath + "arrayListSetNegativeIndex.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("ArrayList Get Empty List",
                        errorPath + "arrayListGetEmptyList.mt",
                        TestType::ERROR_EXPECTED);
    }
}
